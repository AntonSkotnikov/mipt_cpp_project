#include "SocketTransport.hpp"

#include "Client_ServerAPI.hpp"
#include "logger.hpp"

#include <cerrno>
#include <chrono>
#include <cstring>
#include <fcntl.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>

namespace plague {

namespace {

constexpr int kConnectTimeoutMs = 5000;

}  // namespace

SocketTransport::~SocketTransport() {
    disconnect();
}

bool SocketTransport::connectToServer(const char* host, int port) {
    LOG_INFO("Opening socket connection to %s:%d", host, port);
    disconnect();

    struct addrinfo hints {};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    struct addrinfo* results = nullptr;
    const std::string port_text = std::to_string(port);
    const int address_status = getaddrinfo(host, port_text.c_str(), &hints, &results);
    if (address_status != 0) {
        LOG_WARNING("Failed to resolve server address %s:%d: %s",
                    host,
                    port,
                    gai_strerror(address_status));
        return false;
    }

    const auto connect_deadline =
        std::chrono::steady_clock::now() + std::chrono::milliseconds(kConnectTimeoutMs);

    for (struct addrinfo* current = results; current != nullptr; current = current->ai_next) {
        const auto now = std::chrono::steady_clock::now();
        if (now >= connect_deadline) {
            errno = ETIMEDOUT;
            break;
        }

        const int fd = socket(current->ai_family, current->ai_socktype, current->ai_protocol);
        if (fd < 0) {
            LOG_DEBUG("Skipping address candidate: socket failed: %s", std::strerror(errno));
            continue;
        }

        if (!setNonBlocking(fd)) {
            LOG_DEBUG("Skipping address candidate: non-blocking setup failed: %s", std::strerror(errno));
            close(fd);
            continue;
        }

#ifdef SO_NOSIGPIPE
        int no_sigpipe = 1;
        setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, &no_sigpipe, sizeof(no_sigpipe));
#endif

        const int connect_status = connect(fd, current->ai_addr, current->ai_addrlen);
        const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
            connect_deadline - std::chrono::steady_clock::now());
        if (connect_status == 0 ||
            (connect_status < 0 &&
             (errno == EINPROGRESS || errno == EWOULDBLOCK) &&
             waitForConnect(fd, static_cast<int>(remaining.count())))) {
            socket_fd_ = fd;
            connected_ = true;
            LOG_INFO("Socket connection established to %s:%d", host, port);
            break;
        }

        LOG_DEBUG("Address candidate failed for %s:%d: %s", host, port, std::strerror(errno));
        close(fd);
    }

    freeaddrinfo(results);
    if (!connected_) {
        LOG_WARNING("Could not connect to %s:%d", host, port);
    }
    return connected_;
}

void SocketTransport::disconnect() {
    const bool was_connected = connected_ || socket_fd_ >= 0;
    if (socket_fd_ >= 0) {
        close(socket_fd_);
        socket_fd_ = -1;
    }

    connected_ = false;
    read_buffer_.clear();
    parsed_responses_ = {};

    if (was_connected) {
        LOG_INFO("Socket disconnected");
    }
}

bool SocketTransport::isConnected() const {
    return connected_;
}

bool SocketTransport::send(const ClientPackage& package) {
    if (!connected_) {
        LOG_WARNING("Cannot send package: socket is not connected");
        return false;
    }

    LOG_DEBUG("Sending client package: command=%s payload=%s",
              clientCommandToString(package.command),
              package.payload.c_str());
    return sendAll(serializeClientPackage(package.command, package.payload));
}

bool SocketTransport::pollResponse(ServerResponse& response) {
    if (!parsed_responses_.empty()) {
        response = parsed_responses_.front();
        parsed_responses_.pop();
        return true;
    }

    if (!connected_) {
        return false;
    }

    readAvailableData();
    parseBufferedResponses();

    if (parsed_responses_.empty()) {
        return false;
    }

    response = parsed_responses_.front();
    parsed_responses_.pop();
    return true;
}

bool SocketTransport::sendAll(const std::string& wire_data) {
    std::size_t sent_total = 0;

    while (sent_total < wire_data.size()) {
        const ssize_t sent_now = ::send(socket_fd_,
                                        wire_data.data() + sent_total,
                                        wire_data.size() - sent_total,
                                        0);
        if (sent_now < 0) {
            if (errno == EINTR) {
                continue;
            }

            LOG_ERROR("Socket send failed: %s", std::strerror(errno));
            disconnect();
            return false;
        }

        if (sent_now == 0) {
            LOG_WARNING("Socket send returned zero bytes");
            disconnect();
            return false;
        }

        sent_total += static_cast<std::size_t>(sent_now);
    }

    return true;
}

void SocketTransport::readAvailableData() {
    char buffer[4096];

    while (connected_) {
        const ssize_t bytes_read = recv(socket_fd_, buffer, sizeof(buffer), 0);

        if (bytes_read > 0) {
            read_buffer_.append(buffer, static_cast<std::size_t>(bytes_read));
            continue;
        }

        if (bytes_read == 0) {
            LOG_INFO("Server closed the socket connection");
            disconnect();
            return;
        }

        if (errno == EINTR) {
            continue;
        }

        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return;
        }

        LOG_ERROR("Socket recv failed: %s", std::strerror(errno));
        disconnect();
        return;
    }
}

void SocketTransport::parseBufferedResponses() {
    std::size_t newline_pos = std::string::npos;

    while ((newline_pos = read_buffer_.find('\n')) != std::string::npos) {
        std::string line = read_buffer_.substr(0, newline_pos);
        read_buffer_.erase(0, newline_pos + 1);

        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        if (line.empty()) {
            continue;
        }

        const auto parsed = parseServerResponseLine(line);
        if (parsed.has_value()) {
            LOG_DEBUG("Parsed server response: request_id=%u success=%d",
                      parsed->request_id,
                      parsed->success ? 1 : 0);
            parsed_responses_.push(*parsed);
        } else {
            LOG_WARNING("Failed to parse server response line: %s", line.c_str());
        }
    }
}

bool SocketTransport::setNonBlocking(int fd) {
    const int current_flags = fcntl(fd, F_GETFL, 0);
    if (current_flags < 0) {
        return false;
    }

    return fcntl(fd, F_SETFL, current_flags | O_NONBLOCK) == 0;
}

bool SocketTransport::waitForConnect(int fd, int timeout_ms) {
    if (timeout_ms <= 0) {
        errno = ETIMEDOUT;
        return false;
    }

    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);

    while (true) {
        const auto now = std::chrono::steady_clock::now();
        if (now >= deadline) {
            errno = ETIMEDOUT;
            return false;
        }

        const auto remaining = std::chrono::duration_cast<std::chrono::microseconds>(deadline - now);
        timeval timeout {};
        timeout.tv_sec = static_cast<long>(remaining.count() / 1000000);
        timeout.tv_usec = static_cast<int>(remaining.count() % 1000000);

        fd_set write_fds;
        FD_ZERO(&write_fds);
        FD_SET(fd, &write_fds);

        const int ready = select(fd + 1, nullptr, &write_fds, nullptr, &timeout);
        if (ready > 0) {
            int socket_error = 0;
            socklen_t socket_error_size = sizeof(socket_error);
            if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &socket_error, &socket_error_size) < 0) {
                return false;
            }
            if (socket_error != 0) {
                errno = socket_error;
                return false;
            }
            return true;
        }

        if (ready == 0) {
            errno = ETIMEDOUT;
            return false;
        }

        if (errno != EINTR) {
            return false;
        }
    }
}

}
