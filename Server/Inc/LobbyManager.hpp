#pragma once

#include "Client_ServerAPI.hpp"
#include "EventGenerator.hpp"
#include "GameTypes.hpp"
#include "MatModel.hpp"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
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
    LobbyActionResult purchaseUpgrade(ClientSession& session, const std::string& upgradeId);
    void removePlayer(ClientSession& session);

private:
    struct Room {
        std::string name;
        std::string password;
        ClientSession* players[2]{nullptr, nullptr};
        std::atomic_bool gameLoopRunning{false};
        std::thread gameThread;
        World world;
        EventGenerator eventGenerator;
        std::vector<bool> highlightedCountries;
        bool worldInitialized = false;
        bool initialInfectionSelected = false;
        bool activeDnaClickAvailable = false;
        std::size_t activeDnaCountry = 0;
        int activeDnaAmount = 0;
        std::uint64_t activeDnaEventId = 0;
        int gameDay = 1;
    };

    Room* roomForSessionLocked(const ClientSession& session) const;
    Room* findRoomLocked(const std::string& roomName) const;
    ClientSession* opponentOf(const Room& room, ClientSession& session) const;
    ClientSession* opponentOf(ClientSession& session) const;
    bool containsSession(const ClientSession& session) const;
    bool hasTwoPlayers(const Room& room) const;
    bool bothReady(const Room& room) const;
    bool roomIsFullLocked(const Room& room) const;
    void removeBrowserLocked(ClientSession& session);
    LobbyActionResult addPlayerToRoom(ClientSession& session, const std::string& roomName);
    std::string roomListPayloadLocked(const char* event) const;
    void swapRolesLocked(Room& room);
    std::string lobbyPayloadFor(const Room& room, const ClientSession& session, const char* event) const;
    std::string startPayloadFor(const Room& room, const ClientSession& session) const;
    std::string upgradePurchasePayloadFor(const ClientSession& session,
                                          const char* event,
                                          const std::string& upgradeId,
                                          const char* reason) const;
    std::string gameStatsPayload(int tick) const;
    std::string gameStatsPayloadLocked(const Room& room, const char* event, const ClientSession* session = nullptr) const;
    void appendNewsAndEventsLocked(std::ostringstream& payload, const Room* room) const;
    void notifySession(ClientSession& session, const std::string& payload);
    void notifyOpponent(ClientSession& session, const std::string& payload);
    void notifyBoth(Room& room, const std::string& payload);
    void startGameLocked(Room& room, ClientSession& triggeringSession);
    void gameLoop(Room* room);
    void stopGameLoop();

private:
    mutable std::mutex mutex_;
    std::vector<std::unique_ptr<Room>> rooms_;
    std::vector<ClientSession*> roomBrowsers_;
};

}  // namespace plague
