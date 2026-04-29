#pragma once

#include "Client_ServerAPI.hpp"
#include "GameTypes.hpp"

#include <string>

namespace plague {

struct ClientSession {
    int socket_fd = -1;
    std::string readBuffer;
    PlayerRole role = PlayerRole::Humanity;
    int points = 100;
};

class PlagueServer {
public:
    PlagueServer(const std::string& ip, int port);
    ~PlagueServer();
    void run();

private:
    void handleClient(int client_socket);
    bool processInput(ClientSession& session, const std::string& line);
    bool sendResponse(int client_socket, const ServerResponse& response);

private:
    std::string ip_;
    int port_ = 0;
    int server_socket_ = -1;
};

}
