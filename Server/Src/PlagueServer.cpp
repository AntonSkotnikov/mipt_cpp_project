#include "PlagueServer.hpp"

#include "LobbyManager.hpp"
#include "logger.hpp"

#include <algorithm>
#include <arpa/inet.h>
#include <cerrno>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <optional>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

namespace plague {

namespace {

std::string toLower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return text;
}

bool contains(const std::string& text, const char* token) {
    return text.find(token) != std::string::npos;
}

std::optional<std::string> payloadField(const std::string& payload, const std::string& key) {
    const std::string lowerKey = toLower(key);

    std::size_t fieldBegin = 0;
    // Команды лобби используют компактный формат key=value;key=value вместо JSON.
    while (fieldBegin <= payload.size()) {
        std::size_t fieldEnd = payload.find(';', fieldBegin);
        if (fieldEnd == std::string::npos) {
            fieldEnd = payload.size();
        }

        const std::size_t equalsPos = payload.find('=', fieldBegin);
        if (equalsPos != std::string::npos && equalsPos < fieldEnd) {
            const std::string fieldKey = toLower(payload.substr(fieldBegin, equalsPos - fieldBegin));
            if (fieldKey == lowerKey) {
                return payload.substr(equalsPos + 1, fieldEnd - equalsPos - 1);
            }
        }

        if (fieldEnd == payload.size()) {
            break;
        }
        fieldBegin = fieldEnd + 1;
    }

    return std::nullopt;
}

std::string payloadAction(const std::string& payload) {
    if (const auto action = payloadField(payload, "action")) {
        return toLower(*action);
    }
    return {};
}

PlayerSubtype subtypeFromPayload(const std::string& payload, PlayerRole fallbackRole) {
    const std::string lower = toLower(payload);
    if (contains(lower, "virus") || fallbackRole == PlayerRole::Pathogen) {
        return PathogenSubtype::Virus;
    }
    return HumanitySubtype::ResearchInstitute;
}

ServerResponse makeResponse(const ClientRequest& request, const LobbyActionResult& result) {
    ServerResponse response;
    response.request_id = request.request_id;
    response.success = result.success;
    response.payload = result.payload;
    response.error_message = result.errorMessage;
    return response;
}

}  // namespace

PlagueServer::PlagueServer(const std::string& ip, int port)
    : ip_(ip), port_(port), lobby_(std::make_unique<LobbyManager>()) {}

PlagueServer::~PlagueServer() {
    if (server_socket_ >= 0) {
        close(server_socket_);
    }
}

void PlagueServer::run() {
    server_socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket_ < 0) {
        LOG_FATAL("Failed to create server socket");
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
        LOG_FATAL("Invalid bind address: %s", ip_.c_str());
        std::cerr << "Invalid bind address: " << ip_ << '\n';
        std::exit(1);
    }

    if (bind(server_socket_, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0) {
        LOG_FATAL("Bind error on %s:%d: %s", ip_.c_str(), port_, std::strerror(errno));
        std::cerr << "Bind error: " << std::strerror(errno) << '\n';
        std::exit(1);
    }

    if (listen(server_socket_, SOMAXCONN) != 0) {
        LOG_FATAL("Listen error on %s:%d: %s", ip_.c_str(), port_, std::strerror(errno));
        std::cerr << "Listen error: " << std::strerror(errno) << '\n';
        std::exit(1);
    }

    LOG_INFO("Server listening on %s:%d", ip_.c_str(), port_);
    while (true) {
        // У каждого сокета свой ClientSession внутри потока-обработчика.
        const int client_socket = accept(server_socket_, nullptr, nullptr);
        if (client_socket < 0) {
            if (errno == EINTR) {
                continue;
            }
            LOG_ERROR("Accept error: %s", std::strerror(errno));
            continue;
        }

        LOG_INFO("Accepted client socket: fd=%d", client_socket);

#ifdef SO_NOSIGPIPE
        int no_sigpipe_client = 1;
        setsockopt(client_socket, SOL_SOCKET, SO_NOSIGPIPE, &no_sigpipe_client, sizeof(no_sigpipe_client));
#endif

        std::thread(&PlagueServer::handleClient, this, client_socket).detach();
    }
}

void PlagueServer::handleClient(int client_socket) {
    LOG_INFO("Client handler started: fd=%d", client_socket);
    ClientSession session;
    session.socket_fd = client_socket;

    char buffer[4096];

    while (true) {
        const ssize_t bytes_read = recv(client_socket, buffer, sizeof(buffer), 0);
        if (bytes_read <= 0) {
            break;
        }

        session.readBuffer.append(buffer, static_cast<std::size_t>(bytes_read));

        // Запросы разделены переводом строки; неполные строки остаются в буфере сессии.
        std::size_t newline_pos = std::string::npos;
        while ((newline_pos = session.readBuffer.find('\n')) != std::string::npos) {
            std::string line = session.readBuffer.substr(0, newline_pos);
            session.readBuffer.erase(0, newline_pos + 1);

            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }

            if (!line.empty() && !processInput(session, line)) {
                LOG_INFO("Client requested disconnect: fd=%d", client_socket);
                lobby_->removePlayer(session);
                close(client_socket);
                return;
            }
        }
    }

    LOG_INFO("Client connection closed: fd=%d", client_socket);
    lobby_->removePlayer(session);
    close(client_socket);
}

bool PlagueServer::processInput(ClientSession& session, const std::string& line) {
    const auto request = parseClientRequestLine(line);
    if (!request.has_value()) {
        LOG_WARNING("Invalid request format: %s", line.c_str());
        return true;
    }

    LOG_INFO("Processing client request: id=%u command=%s payload=%s",
             request->request_id,
             clientCommandToString(request->command),
             request->payload.c_str());

    ServerResponse response;
    response.request_id = request->request_id;
    response.success = true;
    // Пуш-уведомления переиспользуют последний id запроса, пока в протоколе нет отдельного кадра.
    session.lastRequestId = request->request_id;

    const std::string action = payloadAction(request->payload);

    switch (request->command) {
    case ClientCommand::Connect: {
        response = makeResponse(*request, lobby_->listRooms(session));
        break;
    }

    case ClientCommand::Disconnect:
        LOG_INFO("Disconnect request received: id=%u", request->request_id);
        response.payload = "disconnected";
        sendResponse(session, response);
        return false;

    case ClientCommand::ListRooms:
        response = makeResponse(*request, lobby_->listRooms(session));
        break;

    case ClientCommand::CreateRoom:
        response = makeResponse(*request, lobby_->createRoom(
            session,
            payloadField(request->payload, "room").value_or(""),
            payloadField(request->payload, "password").value_or("")));
        break;

    case ClientCommand::JoinRoom:
        response = makeResponse(*request, lobby_->joinRoom(
            session,
            payloadField(request->payload, "room").value_or(""),
            payloadField(request->payload, "password").value_or("")));
        break;

    case ClientCommand::SelectSubtype:
        response = makeResponse(*request, lobby_->updateSubtype(
            session,
            subtypeFromPayload(request->payload, session.role)));
        break;

    case ClientCommand::ChangeSide:
        response = makeResponse(*request, lobby_->requestSideChange(session));
        break;

    case ClientCommand::Ready:
        response = makeResponse(*request, lobby_->toggleReady(session));
        break;

    case ClientCommand::SelectCountry:
        LOG_INFO("SelectCountry request received: id=%u payload=%s",
                 request->request_id,
                 request->payload.c_str());
        response = makeResponse(*request, lobby_->selectCountry(
            session,
            payloadField(request->payload, "country").value_or("")));
        break;

    case ClientCommand::PurchaseUpgrade:
        LOG_INFO("PurchaseUpgrade request received: id=%u payload=%s",
                 request->request_id,
                 request->payload.c_str());
        response = makeResponse(*request, lobby_->purchaseUpgrade(
            session,
            payloadField(request->payload, "upgrade").value_or("")));
        break;

    case ClientCommand::ChooseHumanity:
    case ClientCommand::ChoosePathogen:
        // Старые клиенты слали действия лобби через команды выбора роли,
        // пока в Common не появились отдельные команды лобби.
        if (action == "ready" || request->payload.empty()) {
            response = makeResponse(*request, lobby_->toggleReady(session));
        } else if (action == "selectsubtype") {
            response = makeResponse(*request, lobby_->updateSubtype(
                session,
                subtypeFromPayload(request->payload, session.role)));
        } else {
            response.success = false;
            response.error_message = "Unsupported choosing-side action";
            response.payload = R"({"screen":"ChoosingSide","event":"UnsupportedAction"})";
        }
        break;

    case ClientCommand::Ping:
        // Старые клиенты туннелировали действия лобби через Ping.
        if (action == "listrooms") {
            response = makeResponse(*request, lobby_->listRooms(session));
        } else if (action == "createroom") {
            response = makeResponse(*request, lobby_->createRoom(
                session,
                payloadField(request->payload, "room").value_or(""),
                payloadField(request->payload, "password").value_or("")));
        } else if (action == "joinroom") {
            response = makeResponse(*request, lobby_->joinRoom(
                session,
                payloadField(request->payload, "room").value_or(""),
                payloadField(request->payload, "password").value_or("")));
        } else if (action == "selectsubtype") {
            response = makeResponse(*request, lobby_->updateSubtype(
                session,
                subtypeFromPayload(request->payload, session.role)));
        } else if (action == "changeside") {
            response = makeResponse(*request, lobby_->requestSideChange(session));
        } else if (action == "ready") {
            response = makeResponse(*request, lobby_->toggleReady(session));
        } else if (action == "purchaseupgrade") {
            response = makeResponse(*request, lobby_->purchaseUpgrade(
                session,
                payloadField(request->payload, "upgrade").value_or("")));
        } else {
            response.payload = "pong";
        }
        break;
    }

    LOG_DEBUG("Sending response: id=%u success=%d payload=%s error=%s",
              response.request_id,
              response.success ? 1 : 0,
              response.payload.c_str(),
              response.error_message.c_str());
    return sendResponse(session, response);
}

bool PlagueServer::sendResponse(ClientSession& session, const ServerResponse& response) {
    const std::string wire = serializeServerResponse(response);
    std::size_t sent_total = 0;

    std::lock_guard<std::mutex> lock(session.sendMutex);

    // Несколько потоков могут писать в один сокет; сериализуем запись и досылаем кадр.
    while (sent_total < wire.size()) {
        const ssize_t sent_now = ::send(session.socket_fd,
                                        wire.data() + sent_total,
                                        wire.size() - sent_total,
                                        0);
        if (sent_now < 0) {
            if (errno == EINTR) {
                continue;
            }

            LOG_ERROR("Send error: %s", std::strerror(errno));
            return false;
        }

        if (sent_now == 0) {
            LOG_WARNING("Send returned zero bytes");
            return false;
        }

        sent_total += static_cast<std::size_t>(sent_now);
    }

    return true;
}

}
