#pragma once

#include "ClientPackage.hpp"
#include "Client_ServerAPI.hpp"

#include <queue>
#include <string>

namespace plague {

// Неблокирующий TCP-транспорт для строкового протокола клиент-сервер.
class SocketTransport final {
public:
    ~SocketTransport();

    bool connectToServer(const char* host, int port);
    void disconnect();
    bool isConnected() const;
    bool send(const ClientPackage& package);
    bool pollResponse(ServerResponse& response);

private:
    bool sendAll(const std::string& wire_data);
    void readAvailableData();
    void parseBufferedResponses();
    bool setNonBlocking(int fd);
    bool waitForConnect(int fd, int timeout_ms);

private:
    int socket_fd_ = -1;
    bool connected_ = false;
    std::string read_buffer_;
    std::queue<ServerResponse> parsed_responses_;
};

}
