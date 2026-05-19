#include "LobbyManager.hpp"

#include "PlagueServer.hpp"
#include "UpgradeCatalog.hpp"
#include "logger.hpp"

#include <algorithm>
#include <chrono>
#include <cerrno>
#include <cmath>
#include <cstring>
#include <cstdint>
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

const char* upgradeCategoryToJson(UpgradeCategory category) {
    switch (category) {
    case UpgradeCategory::Transmission:
        return "Transmission";
    case UpgradeCategory::Clinic:
        return "Clinic";
    case UpgradeCategory::Abilities:
        return "Abilities";
    }
    return "Transmission";
}

std::string jsonEscape(std::string_view text) {
    std::string escaped;
    escaped.reserve(text.size());

    for (const char ch : text) {
        switch (ch) {
        case '\\':
            escaped += R"(\\)";
            break;
        case '"':
            escaped += R"(\")";
            break;
        case '\n':
            escaped += R"(\n)";
            break;
        case '\r':
            escaped += R"(\r)";
            break;
        case '\t':
            escaped += R"(\t)";
            break;
        default:
            escaped.push_back(ch);
            break;
        }
    }

    return escaped;
}

std::uint64_t nonNegativeCount(double value) {
    if (value <= 0.0) {
        return 0;
    }
    return static_cast<std::uint64_t>(std::llround(value));
}

void appendStringArray(std::ostringstream& payload, const std::vector<std::string>& values) {
    payload << '[';
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (i > 0) {
            payload << ',';
        }
        payload << '"' << jsonEscape(values[i]) << '"';
    }
    payload << ']';
}

void appendBoolArray(std::ostringstream& payload, const std::vector<bool>& values) {
    payload << '[';
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (i > 0) {
            payload << ',';
        }
        payload << (values[i] ? "true" : "false");
    }
    payload << ']';
}

void appendUpgrade(std::ostringstream& payload, const UpgradeDefinition& upgrade) {
    payload << R"({"id":")" << jsonEscape(upgrade.id) << '"'
            << R"(,"category":")" << upgradeCategoryToJson(upgrade.category) << '"'
            << R"(,"title":")" << jsonEscape(upgrade.title) << '"'
            << R"(,"cost":)" << upgrade.cost
            << R"(,"description":")" << jsonEscape(upgrade.description) << '"'
            << R"(,"dependencies":)";
    appendStringArray(payload, upgrade.dependencies);
    payload << '}';
}

std::string newsText(const GameNews& news) {
    std::ostringstream text;
    if (news.day > 0) {
        text << "Day " << news.day << ": ";
    }
    text << news.title;
    if (!news.message.empty()) {
        text << " - " << news.message;
    }
    return text.str();
}

const UpgradeDefinition* findUpgrade(PlayerRole role, const UpgradeId& upgradeId) {
    const std::vector<UpgradeDefinition>& upgrades = availableUpgradesFor(role);
    const auto it = std::find_if(upgrades.begin(), upgrades.end(),
        [&upgradeId](const UpgradeDefinition& upgrade) {
            return upgrade.id == upgradeId;
        });
    return it == upgrades.end() ? nullptr : &*it;
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

LobbyActionResult LobbyManager::listRooms(ClientSession& session) {
    LobbyActionResult result;
    std::lock_guard<std::mutex> lock(mutex_);
    session.connected = true;
    if (!containsSession(session) &&
        std::find(roomBrowsers_.begin(), roomBrowsers_.end(), &session) == roomBrowsers_.end()) {
        roomBrowsers_.push_back(&session);
    }
    result.payload = roomListPayloadLocked("RoomList");
    return result;
}

LobbyActionResult LobbyManager::createRoom(ClientSession& session,
                                           const std::string& roomName,
                                           const std::string& password) {
    LobbyActionResult result;

    {
        std::lock_guard<std::mutex> lock(mutex_);

        if (containsSession(session)) {
            Room* currentRoom = roomForSessionLocked(session);
            result.payload = currentRoom != nullptr
                ? lobbyPayloadFor(*currentRoom, session, "AlreadyConnected")
                : roomListPayloadLocked("AlreadyConnected");
            return result;
        }

        if (roomName.empty()) {
            result.success = false;
            result.errorMessage = "Room name is empty";
            result.payload = roomListPayloadLocked("CreateRoomRejected");
            return result;
        }

        if (findRoomLocked(roomName) != nullptr) {
            result.success = false;
            result.errorMessage = "Room already exists";
            result.payload = roomListPayloadLocked("CreateRoomRejected");
            return result;
        }

        auto room = std::make_unique<Room>();
        room->name = roomName;
        room->password = password;
        LOG_INFO("Room created: name=%s private=%d",
                 room->name.c_str(),
                 room->password.empty() ? 0 : 1);

        rooms_.push_back(std::move(room));
    }

    return addPlayerToRoom(session, roomName);
}

LobbyActionResult LobbyManager::joinRoom(ClientSession& session,
                                         const std::string& roomName,
                                         const std::string& password) {
    {
        std::lock_guard<std::mutex> lock(mutex_);

        if (containsSession(session)) {
            LobbyActionResult result;
            Room* currentRoom = roomForSessionLocked(session);
            result.payload = currentRoom != nullptr
                ? lobbyPayloadFor(*currentRoom, session, "AlreadyConnected")
                : roomListPayloadLocked("AlreadyConnected");
            return result;
        }

        Room* room = findRoomLocked(roomName);
        if (room == nullptr) {
            LobbyActionResult result;
            result.success = false;
            result.errorMessage = "Room does not exist";
            result.payload = roomListPayloadLocked("JoinRoomRejected");
            return result;
        }

        if (!room->password.empty() && room->password != password) {
            LobbyActionResult result;
            result.success = false;
            result.errorMessage = "Wrong room password";
            result.payload = roomListPayloadLocked("JoinRoomRejected");
            return result;
        }

        if (roomIsFullLocked(*room)) {
            LobbyActionResult result;
            result.success = false;
            result.errorMessage = "Room is full";
            result.payload = roomListPayloadLocked("JoinRoomRejected");
            return result;
        }
    }

    return addPlayerToRoom(session, roomName);
}

LobbyActionResult LobbyManager::addPlayer(ClientSession& session) {
    std::string roomName;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& candidate : rooms_) {
            if (!roomIsFullLocked(*candidate)) {
                roomName = candidate->name;
                break;
            }
        }
    }
    if (roomName.empty()) {
        LobbyActionResult result;
        result.success = false;
        result.errorMessage = "No available room";
        std::lock_guard<std::mutex> lock(mutex_);
        result.payload = roomListPayloadLocked("JoinRoomRejected");
        return result;
    }
    return addPlayerToRoom(session, roomName);
}

LobbyActionResult LobbyManager::addPlayerToRoom(ClientSession& session, const std::string& roomName) {
    ClientSession* opponent = nullptr;
    std::vector<ClientSession*> roomBrowsers;
    std::string roomBrowsersPayload;
    std::string opponentPayload;
    LobbyActionResult result;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        Room* roomPtr = findRoomLocked(roomName);
        if (roomPtr == nullptr) {
            result.success = false;
            result.errorMessage = "Room does not exist";
            result.payload = roomListPayloadLocked("JoinRoomRejected");
            return result;
        }
        Room& room = *roomPtr;

        if (containsSession(session)) {
            LOG_INFO("Player is already connected to lobby: fd=%d", session.socket_fd);
            Room* currentRoom = roomForSessionLocked(session);
            result.payload = currentRoom != nullptr
                ? lobbyPayloadFor(*currentRoom, session, "AlreadyConnected")
                : roomListPayloadLocked("AlreadyConnected");
            return result;
        }

        if (roomIsFullLocked(room)) {
            LOG_WARNING("Room is full, rejecting player: fd=%d room=%s", session.socket_fd, room.name.c_str());
            result.success = false;
            result.errorMessage = "Room is full";
            result.payload = roomListPayloadLocked("JoinRoomRejected");
            return result;
        }

        removeBrowserLocked(session);
        session.connected = true;
        session.points = 0;
        session.isReady = false;
        session.wantsSideChange = false;
        session.purchasedUpgrades.clear();
        session.roomName = room.name;

        if (room.players[0] == nullptr) {
            static std::mt19937 rng{std::random_device{}()};
            session.role = std::uniform_int_distribution<int>(0, 1)(rng) == 0
                ? PlayerRole::Humanity
                : PlayerRole::Pathogen;
            session.chosenSubtype = defaultSubtypeFor(session.role);
            session.hasChosenSubtype = true;
            session.lobbyState = LobbyState::WaitingForSecond;
            room.players[0] = &session;
            LOG_INFO("Player joined room as first player: fd=%d room=%s role=%s",
                     session.socket_fd,
                     room.name.c_str(),
                     roleToJson(session.role));
        } else {
            session.role = oppositeRole(room.players[0]->role);
            session.chosenSubtype = defaultSubtypeFor(session.role);
            session.hasChosenSubtype = true;
            session.lobbyState = LobbyState::ChoosingSubtype;
            room.players[1] = &session;

            room.players[0]->lobbyState = LobbyState::ChoosingSubtype;
            opponent = room.players[0];
            opponentPayload = lobbyPayloadFor(room, *opponent, "OpponentJoined");
            LOG_INFO("Player joined room as second player: fd=%d room=%s role=%s",
                     session.socket_fd,
                     room.name.c_str(),
                     roleToJson(session.role));
        }

        result.payload = lobbyPayloadFor(room, session, "RoleAssigned");
        roomBrowsers = roomBrowsers_;
        roomBrowsersPayload = roomListPayloadLocked("RoomList");
    }

    if (opponent != nullptr) {
        notifySession(*opponent, opponentPayload);
    }
    for (ClientSession* browser : roomBrowsers) {
        if (browser != nullptr) {
            notifySession(*browser, roomBrowsersPayload);
        }
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
        Room* room = roomForSessionLocked(session);
        if (room == nullptr) {
            LOG_WARNING("Subtype update rejected: player is not in lobby fd=%d", session.socket_fd);
            result.success = false;
            result.errorMessage = "Player is not in lobby";
            return result;
        }

        session.chosenSubtype = subtype;
        session.hasChosenSubtype = true;
        session.isReady = false;
        session.lobbyState = hasTwoPlayers(*room) ? LobbyState::ReadyCheck : LobbyState::WaitingForSecond;
        LOG_INFO("Player selected subtype: fd=%d role=%s subtype=%s",
                 session.socket_fd,
                 roleToJson(session.role),
                 subtypeToJson(session.chosenSubtype));

        result.payload = lobbyPayloadFor(*room, session, "SubtypeSelected");
        opponent = opponentOf(*room, session);
        if (opponent != nullptr) {
            opponentPayload = lobbyPayloadFor(*room, *opponent, "OpponentSelectedSubtype");
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
    Room* roomToStart = nullptr;
    ClientSession* opponent = nullptr;
    std::string opponentPayload;

    {
        std::lock_guard<std::mutex> lock(mutex_);

        // Ready is a synchronized lobby action: both players must be present, both must
        // have selected a subtype, and only then the manager sends the GameStart packet.
        Room* room = roomForSessionLocked(session);
        if (room == nullptr) {
            LOG_WARNING("Ready toggle rejected: player is not in lobby fd=%d", session.socket_fd);
            result.success = false;
            result.errorMessage = "Player is not in lobby";
            return result;
        }

        if (!hasTwoPlayers(*room)) {
            LOG_INFO("Ready toggle delayed: waiting for second player fd=%d", session.socket_fd);
            result.errorMessage = "Waiting for the second player";
            result.payload = lobbyPayloadFor(*room, session, "WaitingForSecond");
            return result;
        }

        if (!session.hasChosenSubtype) {
            LOG_WARNING("Ready toggle rejected: subtype is not selected fd=%d", session.socket_fd);
            result.errorMessage = "Subtype is not selected";
            result.payload = lobbyPayloadFor(*room, session, "SubtypeRequired");
            return result;
        }

        session.isReady = !session.isReady;
        session.lobbyState = LobbyState::ReadyCheck;
        shouldStart = bothReady(*room);
        LOG_INFO("Player ready state changed: fd=%d ready=%d",
                 session.socket_fd,
                 session.isReady ? 1 : 0);

        if (shouldStart) {
            LOG_INFO("Both players are ready, starting game");
            startGameLocked(*room, session);
            result.payload = startPayloadFor(*room, session);
            roomToStart = room;
        } else {
            result.payload = lobbyPayloadFor(*room, session, session.isReady ? "LocalReady" : "LocalNotReady");
            opponent = opponentOf(*room, session);
            if (opponent != nullptr) {
                opponentPayload = lobbyPayloadFor(*room, *opponent, session.isReady ? "OpponentReady" : "OpponentNotReady");
            }
        }
    }

    if (!shouldStart && opponent != nullptr) {
        notifySession(*opponent, opponentPayload);
    }

    if (shouldStart) {
        if (roomToStart->gameThread.joinable()) {
            roomToStart->gameLoopRunning.store(false);
            roomToStart->gameThread.join();
        }
        roomToStart->gameLoopRunning.store(true);
        roomToStart->gameThread = std::thread(&LobbyManager::gameLoop, this, roomToStart);
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

        Room* room = roomForSessionLocked(requester);
        if (room == nullptr) {
            LOG_WARNING("Side-change request rejected: player is not in lobby fd=%d", requester.socket_fd);
            result.success = false;
            result.errorMessage = "Player is not in lobby";
            return result;
        }

        if (!hasTwoPlayers(*room)) {
            LOG_INFO("Side-change request delayed: waiting for second player fd=%d", requester.socket_fd);
            result.errorMessage = "Waiting for the second player";
            result.payload = lobbyPayloadFor(*room, requester, "WaitingForSecond");
            return result;
        }

        requester.wantsSideChange = true;
        opponent = opponentOf(*room, requester);

        if (opponent != nullptr && opponent->wantsSideChange) {
            swapRolesLocked(*room);
            swapped = true;
            LOG_INFO("Both players requested side change, roles swapped");
            result.payload = lobbyPayloadFor(*room, requester, "SidesChanged");
        } else {
            LOG_INFO("Player requested side change: fd=%d", requester.socket_fd);
            result.payload = lobbyPayloadFor(*room, requester, "SideChangeRequested");
        }

        if (opponent != nullptr) {
            opponentPayload = lobbyPayloadFor(*room, *opponent, swapped ? "SidesChanged" : "OpponentRequestsSideChange");
        }
    }

    if (opponent != nullptr) {
        notifySession(*opponent, opponentPayload);
    }

    return result;
}

LobbyActionResult LobbyManager::selectCountry(ClientSession& session, const std::string& countryName) {
    LobbyActionResult result;
    ClientSession* opponent = nullptr;
    std::string opponentPayload;

    {
        std::lock_guard<std::mutex> lock(mutex_);

        Room* room = roomForSessionLocked(session);
        if (room == nullptr) {
            LOG_WARNING("Country selection rejected: player is not in lobby fd=%d", session.socket_fd);
            result.success = false;
            result.errorMessage = "Player is not in lobby";
            return result;
        }

        if (session.lobbyState != LobbyState::InGame) {
            LOG_WARNING("Country selection rejected: game is not running fd=%d country=%s",
                        session.socket_fd,
                        countryName.c_str());
            result.payload = R"({"screen":"Game","event":"CountrySelectionRejected","reason":"game_not_running"})";
            return result;
        }

        if (!room->worldInitialized) {
            room->world = initializeWorld();
            room->worldInitialized = true;
            room->gameDay = 1;
            room->highlightedCountries.assign(room->world.countries.size(), false);
        }

        const int countryIndex = room->world.getCountryIndex(countryName);
        if (countryIndex < 0) {
            LOG_WARNING("Country selection rejected: unknown country=%s fd=%d",
                        countryName.c_str(),
                        session.socket_fd);
            result.payload = R"({"screen":"Game","event":"CountrySelectionRejected","reason":"unknown_country"})";
            return result;
        }

        bool infectedSelectedCountry = false;
        bool dnaCollected = false;
        if (!room->initialInfectionSelected && session.role != PlayerRole::Pathogen) {
            LOG_WARNING("Country infection rejected: role=%s fd=%d country=%s",
                        roleToJson(session.role),
                        session.socket_fd,
                        countryName.c_str());
            result.payload = R"({"screen":"Game","event":"CountrySelectionRejected","reason":"only_pathogen_can_infect"})";
            return result;
        }

        if (!room->initialInfectionSelected) {
            Country& country = room->world.countries[static_cast<std::size_t>(countryIndex)];
            const double infected = std::min(country.pop.susceptible, 1000.0);
            country.pop.susceptible -= infected;
            country.pop.infected += infected;
            room->initialInfectionSelected = true;
            infectedSelectedCountry = infected > 0.0;
            GameNews news(
                EventType::NEWS_FIRST_INFECTION,
                "First Infection!",
                "The virus has been detected in " + country.name + ". " +
                    std::to_string(static_cast<int>(infected)) + " people are infected.",
                country.name,
                static_cast<std::uint64_t>(countryIndex),
                room->world.nextEventId++,
                room->gameDay
            );
            room->world.addNews(news);
            LOG_INFO("Initial infection selected: country=%s role=%s infected=%.0f fd=%d",
                     countryName.c_str(),
                     roleToJson(session.role),
                     infected,
                     session.socket_fd);
        } else {
            LOG_INFO("Country selected: country=%s role=%s fd=%d",
                     countryName.c_str(),
                     roleToJson(session.role),
                     session.socket_fd);

            if (room->activeDnaClickAvailable &&
                room->activeDnaCountry == static_cast<std::size_t>(countryIndex) &&
                room->activeDnaCountry < room->highlightedCountries.size() &&
                room->highlightedCountries[room->activeDnaCountry]) {
                const int dnaReward = room->activeDnaAmount > 0
                    ? room->activeDnaAmount
                    : room->eventGenerator.handleCountryClick(room->activeDnaCountry, 0);
                session.points += dnaReward;
                room->highlightedCountries[room->activeDnaCountry] = false;
                room->activeDnaClickAvailable = false;
                room->activeDnaCountry = 0;
                room->activeDnaAmount = 0;

                Country& country = room->world.countries[static_cast<std::size_t>(countryIndex)];
                GameNews news(
                    EventType::ACTION_DNA_CLICK,
                    "DNA Collected!",
                    std::string(roleToJson(session.role)) + " collected " +
                        std::to_string(dnaReward) + " DNA in " + country.name + ".",
                    country.name,
                    static_cast<std::uint64_t>(countryIndex),
                    room->activeDnaEventId,
                    room->gameDay
                );
                room->world.addNews(news);
                room->activeDnaEventId = 0;

                dnaCollected = true;
                LOG_INFO("DNA click collected: country=%s role=%s reward=%d points=%d fd=%d",
                         countryName.c_str(),
                         roleToJson(session.role),
                         dnaReward,
                         session.points,
                         session.socket_fd);
            }
        }

        result.payload = gameStatsPayloadLocked(*room,
            dnaCollected ? "DNAGranted" : (infectedSelectedCountry ? "CountryInfected" : "CountrySelected"),
            &session);

        opponent = opponentOf(*room, session);
        if (opponent != nullptr) {
            opponentPayload = gameStatsPayloadLocked(*room,
                dnaCollected ? "DNAGranted" : (infectedSelectedCountry ? "CountryInfected" : "CountrySelected"));
        }
    }

    if (opponent != nullptr) {
        notifySession(*opponent, opponentPayload);
    }

    return result;
}

LobbyActionResult LobbyManager::purchaseUpgrade(ClientSession& session, const std::string& upgradeId) {
    LobbyActionResult result;

    {
        std::lock_guard<std::mutex> lock(mutex_);

        Room* room = roomForSessionLocked(session);
        if (room == nullptr) {
            LOG_WARNING("Upgrade purchase rejected: player is not in lobby fd=%d upgrade=%s",
                        session.socket_fd,
                        upgradeId.c_str());
            result.payload = upgradePurchasePayloadFor(session, "UpgradeRejected", upgradeId, "not_in_lobby");
            return result;
        }

        if (session.lobbyState != LobbyState::InGame) {
            LOG_WARNING("Upgrade purchase rejected: game is not running fd=%d upgrade=%s",
                        session.socket_fd,
                        upgradeId.c_str());
            result.payload = upgradePurchasePayloadFor(session, "UpgradeRejected", upgradeId, "game_not_running");
            return result;
        }

        const UpgradeDefinition* upgrade = findUpgrade(session.role, upgradeId);
        if (upgrade == nullptr) {
            LOG_WARNING("Upgrade purchase rejected: unknown upgrade=%s role=%s fd=%d",
                        upgradeId.c_str(),
                        roleToJson(session.role),
                        session.socket_fd);
            result.payload = upgradePurchasePayloadFor(session, "UpgradeRejected", upgradeId, "unknown_upgrade");
            return result;
        }

        if (std::find(session.purchasedUpgrades.begin(),
                      session.purchasedUpgrades.end(),
                      upgradeId) != session.purchasedUpgrades.end()) {
            result.payload = upgradePurchasePayloadFor(session, "UpgradeRejected", upgradeId, "already_purchased");
            return result;
        }

        if (!dependenciesSatisfied(*upgrade, session.purchasedUpgrades)) {
            result.payload = upgradePurchasePayloadFor(session, "UpgradeRejected", upgradeId, "dependencies_not_met");
            return result;
        }

        if (session.points < upgrade->cost) {
            result.payload = upgradePurchasePayloadFor(session, "UpgradeRejected", upgradeId, "not_enough_points");
            return result;
        }

        session.points -= upgrade->cost;
        session.purchasedUpgrades.push_back(upgradeId);
        LOG_INFO("Upgrade purchased: fd=%d upgrade=%s cost=%u points_left=%d",
                 session.socket_fd,
                 upgradeId.c_str(),
                 static_cast<unsigned>(upgrade->cost),
                 session.points);

        result.payload = upgradePurchasePayloadFor(session, "UpgradePurchased", upgradeId, "");
    }

    return result;
}

void LobbyManager::removePlayer(ClientSession& session) {
    ClientSession* opponent = nullptr;
    std::string opponentPayload;
    std::vector<ClientSession*> roomBrowsers;
    std::string roomBrowsersPayload;
    bool wasInGame = false;
    Room* room = nullptr;
    std::unique_ptr<Room> removedRoom;
    std::thread stoppedGameThread;

    {
        std::lock_guard<std::mutex> lock(mutex_);

        // A disconnect during lobby returns the remaining player to waiting; a disconnect
        // during game ends the match for the other client.
        room = roomForSessionLocked(session);
        if (room == nullptr) {
            session.connected = false;
            session.roomName.clear();
            removeBrowserLocked(session);
            LOG_DEBUG("Remove ignored: player is not in lobby fd=%d", session.socket_fd);
            return;
        }

        if (room->players[0] == &session) {
            room->players[0] = nullptr;
        } else if (room->players[1] == &session) {
            room->players[1] = nullptr;
        } else {
            session.connected = false;
            session.roomName.clear();
            removeBrowserLocked(session);
            LOG_DEBUG("Remove ignored: player is not in lobby fd=%d", session.socket_fd);
            return;
        }

        wasInGame = session.lobbyState == LobbyState::InGame;
        session.connected = false;
        session.roomName.clear();
        LOG_INFO("Player removed from lobby: fd=%d wasInGame=%d",
                 session.socket_fd,
                 wasInGame ? 1 : 0);

        opponent = room->players[0] != nullptr ? room->players[0] : room->players[1];
        if (opponent != nullptr) {
            opponent->isReady = false;
            opponent->wantsSideChange = false;
            opponent->lobbyState = LobbyState::WaitingForSecond;
            opponentPayload = wasInGame
                ? R"({"screen":"EndScreen","event":"OpponentDisconnected","winner":"none","reason":"opponent_disconnected"})"
                : lobbyPayloadFor(*room, *opponent, "OpponentDisconnected");
        }

        if (wasInGame) {
            room->gameLoopRunning.store(false);
            if (room->gameThread.joinable()) {
                stoppedGameThread = std::move(room->gameThread);
            }
        }

        if (room->players[0] == nullptr && room->players[1] == nullptr) {
            const auto it = std::find_if(rooms_.begin(), rooms_.end(),
                [room](const std::unique_ptr<Room>& candidate) {
                    return candidate.get() == room;
                });
            if (it != rooms_.end()) {
                (*it)->gameLoopRunning.store(false);
                if ((*it)->gameThread.joinable() && !stoppedGameThread.joinable()) {
                    stoppedGameThread = std::move((*it)->gameThread);
                }
                removedRoom = std::move(*it);
                rooms_.erase(it);
            }
        }

        roomBrowsers = roomBrowsers_;
        roomBrowsersPayload = roomListPayloadLocked("RoomList");
    }

    if (stoppedGameThread.joinable()) {
        LOG_INFO("Stopping game loop because a player disconnected");
        stoppedGameThread.join();
    }

    if (opponent != nullptr) {
        LOG_INFO("Notifying remaining player about opponent disconnect: fd=%d", opponent->socket_fd);
        notifySession(*opponent, opponentPayload);
    }
    for (ClientSession* browser : roomBrowsers) {
        if (browser != nullptr) {
            notifySession(*browser, roomBrowsersPayload);
        }
    }
}

LobbyManager::Room* LobbyManager::roomForSessionLocked(const ClientSession& session) const {
    for (const auto& room : rooms_) {
        if (room->players[0] == &session || room->players[1] == &session) {
            return room.get();
        }
    }
    return nullptr;
}

LobbyManager::Room* LobbyManager::findRoomLocked(const std::string& roomName) const {
    const auto it = std::find_if(rooms_.begin(), rooms_.end(),
        [&roomName](const std::unique_ptr<Room>& room) {
            return room->name == roomName;
        });
    return it == rooms_.end() ? nullptr : it->get();
}

ClientSession* LobbyManager::opponentOf(const Room& room, ClientSession& session) const {
    if (room.players[0] == &session) {
        return room.players[1];
    }
    if (room.players[1] == &session) {
        return room.players[0];
    }
    return nullptr;
}

ClientSession* LobbyManager::opponentOf(ClientSession& session) const {
    Room* room = roomForSessionLocked(session);
    return room == nullptr ? nullptr : opponentOf(*room, session);
}

bool LobbyManager::containsSession(const ClientSession& session) const {
    return roomForSessionLocked(session) != nullptr;
}

bool LobbyManager::hasTwoPlayers(const Room& room) const {
    return room.players[0] != nullptr && room.players[1] != nullptr;
}

bool LobbyManager::bothReady(const Room& room) const {
    return hasTwoPlayers(room) &&
           room.players[0]->isReady &&
           room.players[1]->isReady &&
           room.players[0]->hasChosenSubtype &&
           room.players[1]->hasChosenSubtype;
}

bool LobbyManager::roomIsFullLocked(const Room& room) const {
    return room.players[0] != nullptr && room.players[1] != nullptr;
}

void LobbyManager::removeBrowserLocked(ClientSession& session) {
    roomBrowsers_.erase(
        std::remove(roomBrowsers_.begin(), roomBrowsers_.end(), &session),
        roomBrowsers_.end());
}

std::string LobbyManager::roomListPayloadLocked(const char* event) const {
    std::ostringstream payload;
    payload << R"({"screen":"RoomBrowser")"
            << R"(,"event":")" << event << '"'
            << R"(,"rooms":[)";

    for (std::size_t i = 0; i < rooms_.size(); ++i) {
        const Room& room = *rooms_[i];
        if (i > 0) {
            payload << ',';
        }
        const std::uint16_t players =
            static_cast<std::uint16_t>((room.players[0] != nullptr ? 1 : 0) +
                                       (room.players[1] != nullptr ? 1 : 0));
        payload << R"({"name":")" << jsonEscape(room.name) << '"'
                << R"(,"privateRoom":)" << (room.password.empty() ? "false" : "true")
                << R"(,"players":)" << players
                << R"(,"capacity":2})";
    }

    payload << ']';
    appendNewsAndEventsLocked(payload, nullptr);
    payload << '}';
    return payload.str();
}

void LobbyManager::swapRolesLocked(Room& room) {
    if (!hasTwoPlayers(room)) {
        return;
    }

    std::swap(room.players[0]->role, room.players[1]->role);
    LOG_INFO("Swapping lobby roles");
    room.players[0]->chosenSubtype = defaultSubtypeFor(room.players[0]->role);
    room.players[1]->chosenSubtype = defaultSubtypeFor(room.players[1]->role);

    for (ClientSession* player : room.players) {
        player->hasChosenSubtype = true;
        player->isReady = false;
        player->wantsSideChange = false;
        player->lobbyState = LobbyState::ChoosingSubtype;
    }
}

std::string LobbyManager::lobbyPayloadFor(const Room& room, const ClientSession& session, const char* event) const {
    const ClientSession* opponent = room.players[0] == &session ? room.players[1] : room.players[0];

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

std::string LobbyManager::startPayloadFor(const Room& room, const ClientSession& session) const {
    std::ostringstream payload;
    payload << R"({"screen":"Game")"
            << R"(,"event":"GameStart")"
            << R"(,"role":")" << roleToJson(session.role) << '"'
            << R"(,"subtype":")" << subtypeToJson(session.chosenSubtype) << '"'
            << R"(,"day":)" << room.gameDay
            << R"(,"points":)" << session.points
            << R"(,"availableUpgrades":[)";

    const std::vector<UpgradeDefinition>& upgrades = availableUpgradesFor(session.role);
    for (std::size_t i = 0; i < upgrades.size(); ++i) {
        if (i > 0) {
            payload << ',';
        }
        appendUpgrade(payload, upgrades[i]);
    }

    payload << ']'
            << R"(,"purchasedUpgrades":)";
    appendStringArray(payload, session.purchasedUpgrades);

    payload
            << R"(,"cureProgress":)" << (room.worldInitialized ? room.world.vaccine.progress : 0.0)
            << R"(,"countries":[)";

    if (room.worldInitialized) {
        for (std::size_t i = 0; i < room.world.countries.size(); ++i) {
            if (i > 0) {
                payload << ',';
            }
            const Country& country = room.world.countries[i];
            payload << R"({"name":")" << jsonEscape(country.name) << '"'
                    << R"(,"initial":)" << nonNegativeCount(country.pop.initial)
                    << R"(,"susceptible":)" << nonNegativeCount(country.pop.susceptible)
                    << R"(,"exposed":)" << nonNegativeCount(country.pop.exposed)
                    << R"(,"infected":)" << nonNegativeCount(country.pop.infected)
                    << R"(,"recovered":)" << nonNegativeCount(country.pop.recovered)
                    << R"(,"dead":)" << nonNegativeCount(country.pop.dead)
                    << '}';
        }
    }

    payload << ']';
    appendNewsAndEventsLocked(payload, &room);
    payload << '}';
    return payload.str();
}

std::string LobbyManager::upgradePurchasePayloadFor(const ClientSession& session,
                                                    const char* event,
                                                    const std::string& upgradeId,
                                                    const char* reason) const {
    std::ostringstream payload;
    payload << R"({"screen":"Game")"
            << R"(,"event":")" << event << '"'
            << R"(,"upgradeId":")" << jsonEscape(upgradeId) << '"';

    if (reason != nullptr && reason[0] != '\0') {
        payload << R"(,"reason":")" << reason << '"';
    }

    payload << R"(,"points":)" << session.points
            << R"(,"purchasedUpgrades":)";
    appendStringArray(payload, session.purchasedUpgrades);
    payload << '}';
    return payload.str();
}

std::string LobbyManager::gameStatsPayload(int tick) const {
    (void)tick;
    std::lock_guard<std::mutex> lock(mutex_);
    if (rooms_.empty()) {
        return R"({"screen":"Game","event":"Tick","countries":[],"news":[],"highlightedCountries":[]})";
    }
    return gameStatsPayloadLocked(*rooms_.front(), "Tick");
}

std::string LobbyManager::gameStatsPayloadLocked(const Room& room, const char* event, const ClientSession* session) const {
    std::ostringstream payload;
    payload << R"({"screen":"Game")"
            << R"(,"event":")" << event << '"';

    if (session != nullptr) {
        payload << R"(,"points":)" << session->points;
    }

    payload << R"(,"day":)" << room.gameDay
            << R"(,"cureProgress":)" << (room.worldInitialized ? room.world.vaccine.progress : 0.0)
            << R"(,"countries":[)";

    if (room.worldInitialized) {
        for (std::size_t i = 0; i < room.world.countries.size(); ++i) {
            if (i > 0) {
                payload << ',';
            }
            const Country& country = room.world.countries[i];
            payload << R"({"name":")" << jsonEscape(country.name) << '"'
                    << R"(,"initial":)" << nonNegativeCount(country.pop.initial)
                    << R"(,"susceptible":)" << nonNegativeCount(country.pop.susceptible)
                    << R"(,"exposed":)" << nonNegativeCount(country.pop.exposed)
                    << R"(,"infected":)" << nonNegativeCount(country.pop.infected)
                    << R"(,"recovered":)" << nonNegativeCount(country.pop.recovered)
                    << R"(,"dead":)" << nonNegativeCount(country.pop.dead)
                    << '}';
        }
    }

    payload << ']';
    appendNewsAndEventsLocked(payload, &room);
    payload << '}';
    return payload.str();
}

void LobbyManager::appendNewsAndEventsLocked(std::ostringstream& payload, const Room* room) const {
    std::vector<std::string> news;
    news.reserve(room != nullptr && room->worldInitialized ? room->world.newsQueue.size() : 0);

    if (room != nullptr && room->worldInitialized) {
        for (const GameNews& item : room->world.newsQueue) {
            news.push_back(newsText(item));
        }
    }

    payload << R"(,"news":)";
    appendStringArray(payload, news);
    payload << R"(,"highlightedCountries":)";
    if (room != nullptr) {
        appendBoolArray(payload, room->highlightedCountries);
    } else {
        payload << "[]";
    }
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

void LobbyManager::notifyBoth(Room& room, const std::string& payload) {
    ClientSession* first = nullptr;
    ClientSession* second = nullptr;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        first = room.players[0];
        second = room.players[1];
    }

    if (first != nullptr) {
        notifySession(*first, payload);
    }
    if (second != nullptr) {
        notifySession(*second, payload);
    }
}

void LobbyManager::startGameLocked(Room& room, ClientSession& triggeringSession) {
    if (!hasTwoPlayers(room)) {
        return;
    }

    for (ClientSession* player : room.players) {
        player->lobbyState = LobbyState::InGame;
        player->isReady = true;
        player->wantsSideChange = false;
        player->points = 0;
    }

    room.world = initializeWorld();
    room.worldInitialized = true;
    room.initialInfectionSelected = false;
    room.highlightedCountries.assign(room.world.countries.size(), false);
    room.activeDnaClickAvailable = false;
    room.activeDnaCountry = 0;
    room.activeDnaAmount = 0;
    room.activeDnaEventId = 0;
    room.gameDay = 1;

    ClientSession* opponent = opponentOf(room, triggeringSession);
    if (opponent != nullptr) {
        LOG_DEBUG("Sending game start to opponent: fd=%d", opponent->socket_fd);
        notifySession(*opponent, startPayloadFor(room, *opponent));
    }

    // The thread is launched after the lobby mutex is released by toggleReady().
}

void LobbyManager::gameLoop(Room* room) {
    if (room == nullptr) {
        return;
    }

    LOG_INFO("Game loop started for room=%s", room->name.c_str());

    // The loop advances the shared world model and broadcasts the latest snapshot.
    for (int tick = 1; room->gameLoopRunning.load(); ++tick) {
        std::this_thread::sleep_for(std::chrono::seconds(3));

        bool stillInGame = false;
        std::string tickPayload;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            stillInGame = hasTwoPlayers(*room) &&
                          room->players[0]->lobbyState == LobbyState::InGame &&
                          room->players[1]->lobbyState == LobbyState::InGame;

            if (stillInGame) {
                if (room->worldInitialized && room->initialInfectionSelected) {
                    simulateDay(room->world);
                    const int pathogenPoints = room->players[0]->role == PlayerRole::Pathogen
                        ? room->players[0]->points
                        : room->players[1]->points;
                    const int humanityPoints = room->players[0]->role == PlayerRole::Humanity
                        ? room->players[0]->points
                        : room->players[1]->points;
                    EventResult event = room->eventGenerator.generateEvent(
                        room->world,
                        room->gameDay,
                        pathogenPoints,
                        humanityPoints);
                    if (event.type == EventType::ACTION_DNA_CLICK &&
                        event.highlightedCountry < room->highlightedCountries.size()) {
                        std::fill(room->highlightedCountries.begin(), room->highlightedCountries.end(), false);
                        room->highlightedCountries[event.highlightedCountry] = true;
                        room->activeDnaClickAvailable = true;
                        room->activeDnaCountry = event.highlightedCountry;
                        room->activeDnaAmount = event.dnaAmount;
                        room->activeDnaEventId = event.eventId;
                    }
                }
                if (room->worldInitialized) {
                    ++room->gameDay;
                }
                tickPayload = gameStatsPayloadLocked(*room, "Tick");
            }
        }

        if (!stillInGame) {
            LOG_INFO("Game loop stopping: players are no longer both in game");
            room->gameLoopRunning.store(false);
            break;
        }

        LOG_DEBUG("Broadcasting game tick: tick=%d", tick);
        notifyBoth(*room, tickPayload);
    }
}

void LobbyManager::stopGameLoop() {
    std::vector<std::thread> threads;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& room : rooms_) {
            room->gameLoopRunning.store(false);
            if (room->gameThread.joinable()) {
                threads.push_back(std::move(room->gameThread));
            }
        }
    }

    for (std::thread& thread : threads) {
        if (thread.joinable()) {
            LOG_INFO("Joining game loop thread");
            thread.join();
        }
    }
}

}  // namespace plague
