#include "LobbyManager.hpp"

#include "PlagueServer.hpp"
#include "logger.hpp"

#include <algorithm>
#include <chrono>
#include <cerrno>
#include <cstring>
#include <random>
#include <sstream>
#include <sys/socket.h>
#include <unistd.h>

namespace plague {

namespace {

const char* roleToJson(PlayerRole role) {
    return role == PlayerRole::Humanity ? "humanity" : "pathogen";
}

const char* subtypeToJson(const PlayerSubtype& subtype) {
    if (std::holds_alternative<PathogenSubtype>(subtype)) {
        return "Virus";
    }
    return "ResearchInstitute";
}

const char* lobbyStateToJson(LobbyState state) {
    switch (state) {
    case LobbyState::WaitingForSecond:
        return "WaitingForSecond";
    case LobbyState::ChoosingSubtype:
        return "ChoosingSubtype";
    case LobbyState::ReadyCheck:
        return "ReadyCheck";
    case LobbyState::InGame:
        return "InGame";
    }
    return "WaitingForSecond";
}

PlayerSubtype defaultSubtypeFor(PlayerRole role) {
    if (role == PlayerRole::Pathogen) {
        return PathogenSubtype::Virus;
    }
    return HumanitySubtype::ResearchInstitute;
}

PlayerRole oppositeRole(PlayerRole role) {
    return role == PlayerRole::Humanity ? PlayerRole::Pathogen : PlayerRole::Humanity;
}

bool sendAll(int socketFd, const std::string& wireData) {
    std::size_t sentTotal = 0;

    while (sentTotal < wireData.size()) {
        const ssize_t sentNow = ::send(socketFd,
                                       wireData.data() + sentTotal,
                                       wireData.size() - sentTotal,
                                       0);
        if (sentNow < 0) {
            if (errno == EINTR) {
                continue;
            }

            LOG_ERROR("Lobby send error: %s", std::strerror(errno));
            return false;
        }

        if (sentNow == 0) {
            LOG_WARNING("Lobby send returned zero bytes");
            return false;
        }

        sentTotal += static_cast<std::size_t>(sentNow);
    }

    return true;
}

void sendSessionPacket(ClientSession& session, const std::string& payload) {
    if (!session.connected || session.socket_fd < 0) {
        LOG_DEBUG("Skipping lobby notification: session is not connected");
        return;
    }

    // Server pushes reuse the last request id known for that client.
    // The current request-response transport does not have a separate push frame yet.
    ServerResponse response;
    response.request_id = session.lastRequestId;
    response.success = true;
    response.payload = payload;

    std::lock_guard<std::mutex> lock(session.sendMutex);
    sendAll(session.socket_fd, serializeServerResponse(response));
}

}  // namespace

LobbyManager::LobbyManager() = default;

LobbyManager::~LobbyManager() {
    stopGameLoop();
}

LobbyActionResult LobbyManager::addPlayer(ClientSession& session) {
    ClientSession* opponent = nullptr;
    std::string opponentPayload;
    LobbyActionResult result;

    {
        std::lock_guard<std::mutex> lock(mutex_);

        // The lobby is intentionally a single 1v1 room for now. The first player gets
        // a random side; the second player receives the remaining side.
        if (containsSession(session)) {
            LOG_INFO("Player is already connected to lobby: fd=%d", session.socket_fd);
            result.payload = lobbyPayloadFor(session, "AlreadyConnected");
            return result;
        }

        if (players_[0] != nullptr && players_[1] != nullptr) {
            LOG_WARNING("Lobby is full, rejecting player: fd=%d", session.socket_fd);
            result.success = false;
            result.errorMessage = "Lobby is full";
            result.payload = R"({"screen":"MainMenu","error":"Lobby is full"})";
            return result;
        }

        session.connected = true;
        session.points = 100;
        session.isReady = false;
        session.wantsSideChange = false;
        session.purchasedUpgrades.clear();

        if (players_[0] == nullptr) {
            static std::mt19937 rng{std::random_device{}()};
            session.role = std::uniform_int_distribution<int>(0, 1)(rng) == 0
                ? PlayerRole::Humanity
                : PlayerRole::Pathogen;
            session.chosenSubtype = defaultSubtypeFor(session.role);
            session.hasChosenSubtype = true;
            session.lobbyState = LobbyState::WaitingForSecond;
            players_[0] = &session;
            LOG_INFO("Player joined lobby as first player: fd=%d role=%s",
                     session.socket_fd,
                     roleToJson(session.role));
        } else {
            session.role = oppositeRole(players_[0]->role);
            session.chosenSubtype = defaultSubtypeFor(session.role);
            session.hasChosenSubtype = true;
            session.lobbyState = LobbyState::ChoosingSubtype;
            players_[1] = &session;

            players_[0]->lobbyState = LobbyState::ChoosingSubtype;
            opponent = players_[0];
            opponentPayload = lobbyPayloadFor(*opponent, "OpponentJoined");
            LOG_INFO("Player joined lobby as second player: fd=%d role=%s",
                     session.socket_fd,
                     roleToJson(session.role));
        }

        result.payload = lobbyPayloadFor(session, "RoleAssigned");
    }

    if (opponent != nullptr) {
        notifySession(*opponent, opponentPayload);
    }

    return result;
}

LobbyActionResult LobbyManager::updateSubtype(ClientSession& session, PlayerSubtype subtype) {
    LobbyActionResult result;
    ClientSession* opponent = nullptr;
    std::string opponentPayload;

    {
        std::lock_guard<std::mutex> lock(mutex_);

        // Subtype selection is stored on the server before Ready is accepted, so a client
        // cannot start the game by sending Ready without first confirming its subtype.
        if (!containsSession(session)) {
            LOG_WARNING("Subtype update rejected: player is not in lobby fd=%d", session.socket_fd);
            result.success = false;
            result.errorMessage = "Player is not in lobby";
            return result;
        }

        session.chosenSubtype = subtype;
        session.hasChosenSubtype = true;
        session.isReady = false;
        session.lobbyState = hasTwoPlayers() ? LobbyState::ReadyCheck : LobbyState::WaitingForSecond;
        LOG_INFO("Player selected subtype: fd=%d role=%s subtype=%s",
                 session.socket_fd,
                 roleToJson(session.role),
                 subtypeToJson(session.chosenSubtype));

        result.payload = lobbyPayloadFor(session, "SubtypeSelected");
        opponent = opponentOf(session);
        if (opponent != nullptr) {
            opponentPayload = lobbyPayloadFor(*opponent, "OpponentSelectedSubtype");
        }
    }

    if (opponent != nullptr) {
        notifySession(*opponent, opponentPayload);
    }
    return result;
}

LobbyActionResult LobbyManager::toggleReady(ClientSession& session) {
    LobbyActionResult result;
    bool shouldStart = false;
    ClientSession* opponent = nullptr;
    std::string opponentPayload;

    {
        std::lock_guard<std::mutex> lock(mutex_);

        // Ready is a synchronized lobby action: both players must be present, both must
        // have selected a subtype, and only then the manager sends the GameStart packet.
        if (!containsSession(session)) {
            LOG_WARNING("Ready toggle rejected: player is not in lobby fd=%d", session.socket_fd);
            result.success = false;
            result.errorMessage = "Player is not in lobby";
            return result;
        }

        if (!hasTwoPlayers()) {
            LOG_INFO("Ready toggle delayed: waiting for second player fd=%d", session.socket_fd);
            result.errorMessage = "Waiting for the second player";
            result.payload = lobbyPayloadFor(session, "WaitingForSecond");
            return result;
        }

        if (!session.hasChosenSubtype) {
            LOG_WARNING("Ready toggle rejected: subtype is not selected fd=%d", session.socket_fd);
            result.errorMessage = "Subtype is not selected";
            result.payload = lobbyPayloadFor(session, "SubtypeRequired");
            return result;
        }

        session.isReady = !session.isReady;
        session.lobbyState = LobbyState::ReadyCheck;
        shouldStart = bothReady();
        LOG_INFO("Player ready state changed: fd=%d ready=%d",
                 session.socket_fd,
                 session.isReady ? 1 : 0);

        if (shouldStart) {
            LOG_INFO("Both players are ready, starting game");
            startGameLocked(session);
            result.payload = startPayloadFor(session);
        } else {
            result.payload = lobbyPayloadFor(session, session.isReady ? "LocalReady" : "LocalNotReady");
            opponent = opponentOf(session);
            if (opponent != nullptr) {
                opponentPayload = lobbyPayloadFor(*opponent, session.isReady ? "OpponentReady" : "OpponentNotReady");
            }
        }
    }

    if (!shouldStart && opponent != nullptr) {
        notifySession(*opponent, opponentPayload);
    }

    if (shouldStart) {
        if (gameThread_.joinable()) {
            gameLoopRunning_.store(false);
            gameThread_.join();
        }
        gameLoopRunning_.store(true);
        gameThread_ = std::thread(&LobbyManager::gameLoop, this);
    }

    return result;
}

LobbyActionResult LobbyManager::requestSideChange(ClientSession& requester) {
    ClientSession* opponent = nullptr;
    std::string opponentPayload;
    LobbyActionResult result;
    bool swapped = false;

    {
        std::lock_guard<std::mutex> lock(mutex_);

        if (!containsSession(requester)) {
            LOG_WARNING("Side-change request rejected: player is not in lobby fd=%d", requester.socket_fd);
            result.success = false;
            result.errorMessage = "Player is not in lobby";
            return result;
        }

        if (!hasTwoPlayers()) {
            LOG_INFO("Side-change request delayed: waiting for second player fd=%d", requester.socket_fd);
            result.errorMessage = "Waiting for the second player";
            result.payload = lobbyPayloadFor(requester, "WaitingForSecond");
            return result;
        }

        requester.wantsSideChange = true;
        opponent = opponentOf(requester);

        if (opponent != nullptr && opponent->wantsSideChange) {
            swapRolesLocked();
            swapped = true;
            LOG_INFO("Both players requested side change, roles swapped");
            result.payload = lobbyPayloadFor(requester, "SidesChanged");
        } else {
            LOG_INFO("Player requested side change: fd=%d", requester.socket_fd);
            result.payload = lobbyPayloadFor(requester, "SideChangeRequested");
        }

        if (opponent != nullptr) {
            opponentPayload = lobbyPayloadFor(*opponent, swapped ? "SidesChanged" : "OpponentRequestsSideChange");
        }
    }

    if (opponent != nullptr) {
        notifySession(*opponent, opponentPayload);
    }

    return result;
}

void LobbyManager::removePlayer(ClientSession& session) {
    ClientSession* opponent = nullptr;
    bool wasInGame = false;

    {
        std::lock_guard<std::mutex> lock(mutex_);

        // A disconnect during lobby returns the remaining player to waiting; a disconnect
        // during game ends the match for the other client.
        if (players_[0] == &session) {
            players_[0] = nullptr;
        } else if (players_[1] == &session) {
            players_[1] = nullptr;
        } else {
            session.connected = false;
            LOG_DEBUG("Remove ignored: player is not in lobby fd=%d", session.socket_fd);
            return;
        }

        wasInGame = session.lobbyState == LobbyState::InGame;
        session.connected = false;
        LOG_INFO("Player removed from lobby: fd=%d wasInGame=%d",
                 session.socket_fd,
                 wasInGame ? 1 : 0);

        opponent = players_[0] != nullptr ? players_[0] : players_[1];
        if (opponent != nullptr) {
            opponent->isReady = false;
            opponent->wantsSideChange = false;
            opponent->lobbyState = LobbyState::WaitingForSecond;
        }
    }

    if (wasInGame) {
        LOG_INFO("Stopping game loop because a player disconnected");
        gameLoopRunning_.store(false);
    }

    if (opponent != nullptr) {
        LOG_INFO("Notifying remaining player about opponent disconnect: fd=%d", opponent->socket_fd);
        notifySession(*opponent,
                      R"({"screen":"EndScreen","event":"OpponentDisconnected","winner":"none","reason":"opponent_disconnected"})");
    }
}

ClientSession* LobbyManager::opponentOf(ClientSession& session) const {
    if (players_[0] == &session) {
        return players_[1];
    }
    if (players_[1] == &session) {
        return players_[0];
    }
    return nullptr;
}

bool LobbyManager::containsSession(const ClientSession& session) const {
    return players_[0] == &session || players_[1] == &session;
}

bool LobbyManager::hasTwoPlayers() const {
    return players_[0] != nullptr && players_[1] != nullptr;
}

bool LobbyManager::bothReady() const {
    return hasTwoPlayers() &&
           players_[0]->isReady &&
           players_[1]->isReady &&
           players_[0]->hasChosenSubtype &&
           players_[1]->hasChosenSubtype;
}

void LobbyManager::swapRolesLocked() {
    if (!hasTwoPlayers()) {
        return;
    }

    std::swap(players_[0]->role, players_[1]->role);
    LOG_INFO("Swapping lobby roles");
    players_[0]->chosenSubtype = defaultSubtypeFor(players_[0]->role);
    players_[1]->chosenSubtype = defaultSubtypeFor(players_[1]->role);

    for (ClientSession* player : players_) {
        player->hasChosenSubtype = true;
        player->isReady = false;
        player->wantsSideChange = false;
        player->lobbyState = LobbyState::ChoosingSubtype;
    }
}

std::string LobbyManager::lobbyPayloadFor(const ClientSession& session, const char* event) const {
    const ClientSession* opponent = players_[0] == &session ? players_[1] : players_[0];

    std::ostringstream payload;
    payload << R"({"screen":"ChoosingSide")"
            << R"(,"event":")" << event << '"'
            << R"(,"role":")" << roleToJson(session.role) << '"'
            << R"(,"subtype":")" << subtypeToJson(session.chosenSubtype) << '"'
            << R"(,"lobbyState":")" << lobbyStateToJson(session.lobbyState) << '"'
            << R"(,"ready":)" << (session.isReady ? "true" : "false")
            << R"(,"sideChangeRequested":)" << (session.wantsSideChange ? "true" : "false")
            << R"(,"opponentConnected":)" << (opponent != nullptr ? "true" : "false")
            << R"(,"opponentReady":)" << (opponent != nullptr && opponent->isReady ? "true" : "false")
            << R"(,"opponentSideChangeRequested":)" << (opponent != nullptr && opponent->wantsSideChange ? "true" : "false")
            << '}';
    return payload.str();
}

std::string LobbyManager::startPayloadFor(const ClientSession& session) const {
    std::ostringstream payload;
    payload << R"({"screen":"Game")"
            << R"(,"event":"GameStart")"
            << R"(,"role":")" << roleToJson(session.role) << '"'
            << R"(,"subtype":")" << subtypeToJson(session.chosenSubtype) << '"'
            << R"(,"day":1)"
            << R"(,"points":)" << session.points
            << R"(,"availableUpgrades":[)";

    if (session.role == PlayerRole::Pathogen) {
        payload << R"({"id":"air_1","category":"Transmission","cost":10},)"
                << R"({"id":"symptom_1","category":"Abilities","cost":15})";
    } else {
        payload << R"({"id":"vaccine_1","category":"Clinic","cost":10},)"
                << R"({"id":"quarantine_1","category":"Transmission","cost":15})";
    }

    payload << R"(]})";
    return payload.str();
}

std::string LobbyManager::gameStatsPayload(int tick) const {
    std::ostringstream payload;
    payload << R"({"screen":"Game")"
            << R"(,"event":"Tick")"
            << R"(,"day":)" << (tick + 1)
            << R"(,"infected":)" << (1000 + tick * 137)
            << R"(,"dead":)" << (tick * 13)
            << R"(,"cureProgress":)" << std::min(100, tick * 3)
            << '}';
    return payload.str();
}

void LobbyManager::notifySession(ClientSession& session, const std::string& payload) {
    LOG_DEBUG("Sending lobby notification: fd=%d payload=%s",
              session.socket_fd,
              payload.c_str());
    sendSessionPacket(session, payload);
}

void LobbyManager::notifyOpponent(ClientSession& session, const std::string& payload) {
    ClientSession* opponent = nullptr;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        opponent = opponentOf(session);
    }

    if (opponent != nullptr) {
        notifySession(*opponent, payload);
    }
}

void LobbyManager::notifyBoth(const std::string& payload) {
    ClientSession* first = nullptr;
    ClientSession* second = nullptr;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        first = players_[0];
        second = players_[1];
    }

    if (first != nullptr) {
        notifySession(*first, payload);
    }
    if (second != nullptr) {
        notifySession(*second, payload);
    }
}

void LobbyManager::startGameLocked(ClientSession& triggeringSession) {
    if (!hasTwoPlayers()) {
        return;
    }

    for (ClientSession* player : players_) {
        player->lobbyState = LobbyState::InGame;
        player->isReady = true;
        player->wantsSideChange = false;
        player->points = 100;
    }

    ClientSession* opponent = opponentOf(triggeringSession);
    if (opponent != nullptr) {
        LOG_DEBUG("Sending game start to opponent: fd=%d", opponent->socket_fd);
        notifySession(*opponent, startPayloadFor(*opponent));
    }

    // The thread is launched after the lobby mutex is released by toggleReady().
}

void LobbyManager::gameLoop() {
    LOG_INFO("Game loop started");

    // This placeholder simulation loop exercises the network contract until a real
    // win condition or a disconnect stops the match.
    for (int tick = 1; gameLoopRunning_.load(); ++tick) {
        std::this_thread::sleep_for(std::chrono::seconds(3));

        bool stillInGame = false;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            stillInGame = hasTwoPlayers() &&
                          players_[0]->lobbyState == LobbyState::InGame &&
                          players_[1]->lobbyState == LobbyState::InGame;
        }

        if (!stillInGame) {
            LOG_INFO("Game loop stopping: players are no longer both in game");
            gameLoopRunning_.store(false);
            break;
        }

        LOG_DEBUG("Broadcasting game tick: tick=%d", tick);
        notifyBoth(gameStatsPayload(tick));
    }
}

void LobbyManager::stopGameLoop() {
    gameLoopRunning_.store(false);
    if (gameThread_.joinable()) {
        LOG_INFO("Joining game loop thread");
        gameThread_.join();
    }
}

}  // namespace plague
