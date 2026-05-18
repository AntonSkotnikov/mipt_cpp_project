#pragma once

#include "Client_ServerAPI.hpp"
#include "GameTypes.hpp"
#include "MatModel.hpp"

#include <atomic>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace plague {

struct ClientSession;

struct LobbyActionResult {
    bool success = true;
    std::string payload;
    std::string errorMessage;
};

class LobbyManager {
public:
    LobbyManager();
    ~LobbyManager();

    LobbyActionResult listRooms(ClientSession& session);
    LobbyActionResult createRoom(ClientSession& session,
                                 const std::string& roomName,
                                 const std::string& password);
    LobbyActionResult joinRoom(ClientSession& session,
                               const std::string& roomName,
                               const std::string& password);
    LobbyActionResult addPlayer(ClientSession& session);
    LobbyActionResult updateSubtype(ClientSession& session, PlayerSubtype subtype);
    LobbyActionResult toggleReady(ClientSession& session);
    LobbyActionResult requestSideChange(ClientSession& requester);
    LobbyActionResult selectCountry(ClientSession& session, const std::string& countryName);
    void removePlayer(ClientSession& session);

private:
    ClientSession* opponentOf(ClientSession& session) const;
    bool containsSession(const ClientSession& session) const;
    bool hasTwoPlayers() const;
    bool bothReady() const;
    bool roomExistsLocked() const;
    bool roomIsFullLocked() const;
    void removeBrowserLocked(ClientSession& session);
    std::string roomListPayloadLocked(const char* event) const;
    void swapRolesLocked();
    std::string lobbyPayloadFor(const ClientSession& session, const char* event) const;
    std::string startPayloadFor(const ClientSession& session) const;
    std::string gameStatsPayload(int tick) const;
    std::string gameStatsPayloadLocked(const char* event) const;
    void notifySession(ClientSession& session, const std::string& payload);
    void notifyOpponent(ClientSession& session, const std::string& payload);
    void notifyBoth(const std::string& payload);
    void startGameLocked(ClientSession& triggeringSession);
    void gameLoop();
    void stopGameLoop();

private:
    mutable std::mutex mutex_;
    ClientSession* players_[2]{nullptr, nullptr};
    std::vector<ClientSession*> roomBrowsers_;
    std::atomic_bool gameLoopRunning_{false};
    std::thread gameThread_;
    std::string roomName_;
    std::string roomPassword_;
    World world_;
    bool worldInitialized_ = false;
    bool initialInfectionSelected_ = false;
    int gameDay_ = 1;
};

}  // namespace plague
