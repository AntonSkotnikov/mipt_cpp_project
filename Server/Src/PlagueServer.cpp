#include "PlagueServer.hpp"

#include <arpa/inet.h>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

namespace plague {

PlagueServer::PlagueServer(const std::string& ip, int port)
    : ip_(ip), port_(port) {}

PlagueServer::~PlagueServer() {
    if (server_socket_ >= 0) {
        close(server_socket_);
    }
}

void PlagueServer::run() {
    server_socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket_ < 0) {
        std::cerr << "Failed to create server socket\n";
        std::exit(1);
    }

    int reuse_addr = 1;
    setsockopt(server_socket_, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof(reuse_addr));
#ifdef SO_NOSIGPIPE
    int no_sigpipe = 1;
    setsockopt(server_socket_, SOL_SOCKET, SO_NOSIGPIPE, &no_sigpipe, sizeof(no_sigpipe));
#endif

    sockaddr_in address {};
    address.sin_family = AF_INET;
    address.sin_port = htons(static_cast<uint16_t>(port_));
    if (inet_pton(AF_INET, ip_.c_str(), &address.sin_addr) != 1) {
        std::cerr << "Invalid bind address: " << ip_ << '\n';
        std::exit(1);
    }

    if (bind(server_socket_, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0) {
        std::cerr << "Bind error: " << std::strerror(errno) << '\n';
        std::exit(1);
    }

    if (listen(server_socket_, SOMAXCONN) != 0) {
        std::cerr << "Listen error: " << std::strerror(errno) << '\n';
        std::exit(1);
    }

    std::cout << "Server listening on " << ip_ << ':' << port_ << '\n';

    while (true) {
        const int client_socket = accept(server_socket_, nullptr, nullptr);
        if (client_socket < 0) {
            if (errno == EINTR) {
                continue;
            }

            std::cerr << "Accept error: " << std::strerror(errno) << '\n';
            continue;
        }

#ifdef SO_NOSIGPIPE
        int no_sigpipe_client = 1;
        setsockopt(client_socket, SOL_SOCKET, SO_NOSIGPIPE, &no_sigpipe_client, sizeof(no_sigpipe_client));
#endif

        std::thread(&PlagueServer::handleClient, this, client_socket).detach();
    }
}

void PlagueServer::handleClient(int client_socket) {
    ClientSession session;
    session.socket_fd = client_socket;

    char buffer[4096];

    while (true) {
        const ssize_t bytes_read = recv(client_socket, buffer, sizeof(buffer), 0);
        if (bytes_read <= 0) {
            break;
        }

        session.readBuffer.append(buffer, static_cast<std::size_t>(bytes_read));

        std::size_t newline_pos = std::string::npos;
        while ((newline_pos = session.readBuffer.find('\n')) != std::string::npos) {
            std::string line = session.readBuffer.substr(0, newline_pos);
            session.readBuffer.erase(0, newline_pos + 1);

            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }

            if (!line.empty() && !processInput(session, line)) {
                close(client_socket);
                return;
            }
        }
    }

    close(client_socket);
}

bool PlagueServer::processInput(ClientSession& session, const std::string& line) {
    const auto request = parseClientRequestLine(line);
    if (!request.has_value()) {
        std::cerr << "Invalid request format: " << line << '\n';
        return true;
    }

    ServerResponse response;
    response.request_id = request->request_id;
    response.success = true;

    switch (request->command) {
    case ClientCommand::Connect:
        response.payload = R"({"screen":"ChoosingSide"})";
        break;

    case ClientCommand::Disconnect:
        response.payload = "disconnected";
        return sendResponse(session.socket_fd, response);

    case ClientCommand::ChooseHumanity:
        session.role = PlayerRole::Humanity;
        response.payload = R"({"role":"humanity","screen":"Game","day":1,"points":100,"news":[]})";
        break;

    case ClientCommand::ChoosePathogen:
        session.role = PlayerRole::Pathogen;
        response.payload = R"({"role":"pathogen","screen":"Game","day":1,"points":100,"news":[]})";
        break;

    case ClientCommand::Ping:
        response.payload = "pong";
        break;
    }

    return sendResponse(session.socket_fd, response);
}

bool PlagueServer::sendResponse(int client_socket, const ServerResponse& response) {
    const std::string wire = serializeServerResponse(response);
    std::size_t sent_total = 0;

    while (sent_total < wire.size()) {
        const ssize_t sent_now = ::send(client_socket,
                                        wire.data() + sent_total,
                                        wire.size() - sent_total,
                                        0);
        if (sent_now < 0) {
            if (errno == EINTR) {
                continue;
            }

            std::cerr << "Send error: " << std::strerror(errno) << '\n';
            return false;
        }

        if (sent_now == 0) {
            return false;
        }

        sent_total += static_cast<std::size_t>(sent_now);
    }

    return true;
}

}
