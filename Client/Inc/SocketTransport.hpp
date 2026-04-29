#pragma once

#include "ITransport.hpp"

#include <queue>
#include <string>

namespace plague {

class SocketTransport final : public ITransport {
public:
    ~SocketTransport() override;

    bool connectToServer(const char* host, int port) override;
    void disconnect() override;
    bool isConnected() const override;
    bool send(const ClientPackage& package) override;
    bool pollResponse(ServerResponse& response) override;

private:
    bool sendAll(const std::string& wire_data);
    void readAvailableData();
    void parseBufferedResponses();
    bool setNonBlocking(int fd);

private:
    int socket_fd_ = -1;
    bool connected_ = false;
    std::string read_buffer_;
    std::queue<ServerResponse> parsed_responses_;
};

}
