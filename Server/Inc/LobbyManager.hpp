#pragma once

#include "Client_ServerAPI.hpp"
#include "GameTypes.hpp"

#include <atomic>
#include <mutex>
#include <string>
#include <thread>

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

    LobbyActionResult addPlayer(ClientSession& session);
    LobbyActionResult updateSubtype(ClientSession& session, PlayerSubtype subtype);
    LobbyActionResult toggleReady(ClientSession& session);
    LobbyActionResult requestSideChange(ClientSession& requester);
    void removePlayer(ClientSession& session);

private:
    ClientSession* opponentOf(ClientSession& session) const;
    bool containsSession(const ClientSession& session) const;
    bool hasTwoPlayers() const;
    bool bothReady() const;
    void swapRolesLocked();
    std::string lobbyPayloadFor(const ClientSession& session, const char* event) const;
    std::string startPayloadFor(const ClientSession& session) const;
    std::string gameStatsPayload(int tick) const;
    void notifySession(ClientSession& session, const std::string& payload);
    void notifyOpponent(ClientSession& session, const std::string& payload);
    void notifyBoth(const std::string& payload);
    void startGameLocked(ClientSession& triggeringSession);
    void gameLoop();
    void stopGameLoop();

private:
    mutable std::mutex mutex_;
    ClientSession* players_[2]{nullptr, nullptr};
    std::atomic_bool gameLoopRunning_{false};
    std::thread gameThread_;
};

}  // namespace plague
