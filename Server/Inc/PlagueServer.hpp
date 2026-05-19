#pragma once

#include "Client_ServerAPI.hpp"
#include "GameTypes.hpp"
#include "Upgrade.hpp"

#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace plague {

class LobbyManager;

enum class LobbyState {
    WaitingForSecond,
    ChoosingSubtype,
    ReadyCheck,
    InGame
};

struct ClientSession {
    int socket_fd = -1;
    std::string readBuffer;
    PlayerRole role = PlayerRole::Humanity;
    PlayerSubtype chosenSubtype = HumanitySubtype::ResearchInstitute;
    bool hasChosenSubtype = false;
    bool isReady = false;
    bool wantsSideChange = false;
    std::vector<UpgradeId> purchasedUpgrades;
    std::string roomName;
    LobbyState lobbyState = LobbyState::WaitingForSecond;
    int points = 0;
    RequestId lastRequestId = 0;
    bool connected = true;
    std::mutex sendMutex;
};

class PlagueServer {
public:
    PlagueServer(const std::string& ip, int port);
    ~PlagueServer();
    void run();

private:
    void handleClient(int client_socket);
    bool processInput(ClientSession& session, const std::string& line);
    bool sendResponse(ClientSession& session, const ServerResponse& response);

private:
    std::string ip_;
    int port_ = 0;
    int server_socket_ = -1;
    std::unique_ptr<LobbyManager> lobby_;
};

}
