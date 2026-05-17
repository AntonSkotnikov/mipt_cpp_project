#include "ClientApp.hpp"

#include "logger.hpp"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>

namespace plague {

namespace {

constexpr const char* kServerHost = "127.0.0.1";
constexpr int kServerPort = 5555;

std::string toLower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return text;
}

bool contains(std::string_view text, std::string_view token) {
    return text.find(token) != std::string_view::npos;
}

std::optional<PlayerRole> parseRoleFromPayload(const std::string& payload) {
    const std::string lower = toLower(payload);
    if (contains(lower, "pathogen")) {
        return PlayerRole::Pathogen;
    }
    if (contains(lower, "humanity")) {
        return PlayerRole::Humanity;
    }
    return std::nullopt;
}

std::optional<int> parseIntField(const std::string& payload, std::string_view key) {
    const std::string lower = toLower(payload);
    const std::string lower_key = toLower(std::string(key));
    std::size_t pos = lower.find(lower_key);
    if (pos == std::string::npos) {
        return std::nullopt;
    }

    pos = lower.find_first_of("=:", pos + lower_key.size());
    if (pos == std::string::npos) {
        return std::nullopt;
    }

    pos = lower.find_first_of("-0123456789", pos + 1);
    if (pos == std::string::npos) {
        return std::nullopt;
    }

    try {
        return std::stoi(lower.substr(pos));
    } catch (...) {
        return std::nullopt;
    }
}

std::optional<bool> parseBoolField(const std::string& payload, std::string_view key) {
    const std::string lower = toLower(payload);
    const std::string lowerKey = toLower(std::string(key));
    const std::string quotedKey = '"' + lowerKey + '"';

    std::size_t pos = lower.find(quotedKey);
    std::size_t keyEnd = std::string::npos;
    if (pos == std::string::npos) {
        for (std::size_t searchFrom = 0; ; searchFrom = pos + lowerKey.size()) {
            pos = lower.find(lowerKey, searchFrom);
            if (pos == std::string::npos) {
                break;
            }

            const bool startsAtBoundary = pos == 0 ||
                (!std::isalnum(static_cast<unsigned char>(lower[pos - 1])) && lower[pos - 1] != '_');
            const std::size_t after = pos + lowerKey.size();
            const bool endsAtBoundary = after >= lower.size() ||
                (!std::isalnum(static_cast<unsigned char>(lower[after])) && lower[after] != '_');
            if (startsAtBoundary && endsAtBoundary) {
                keyEnd = after;
                break;
            }
        }
    } else {
        keyEnd = pos + quotedKey.size();
    }
    if (pos == std::string::npos || keyEnd == std::string::npos) {
        return std::nullopt;
    }

    pos = lower.find_first_of("=:", keyEnd);
    if (pos == std::string::npos) {
        return std::nullopt;
    }

    pos = lower.find_first_not_of(" \t\r\n\"", pos + 1);
    if (pos == std::string::npos) {
        return std::nullopt;
    }

    if (lower.compare(pos, 4, "true") == 0 || lower[pos] == '1') {
        return true;
    }
    if (lower.compare(pos, 5, "false") == 0 || lower[pos] == '0') {
        return false;
    }
    return std::nullopt;
}

bool packetShowsChoosingSide(const std::string& payload) {
    const std::string lower = toLower(payload);
    return contains(lower, "choosingside") ||
           contains(lower, "choosing_side") ||
           contains(lower, "lobby");
}

bool packetStartsGame(const std::string& payload) {
    const std::string lower = toLower(payload);
    return contains(lower, "\"event\":\"gamestart\"") ||
           contains(lower, "event=gamestart") ||
           contains(lower, "gamestart") ||
           contains(lower, "game_start") ||
           (contains(lower, "start") && contains(lower, "game"));
}

bool packetShowsGame(const std::string& payload) {
    const std::string lower = toLower(payload);
    return contains(lower, "\"screen\":\"game\"") ||
           contains(lower, "screen=game");
}

bool packetEndsGame(const std::string& payload) {
    const std::string lower = toLower(payload);
    return contains(lower, "endscreen") ||
           contains(lower, "gameover") ||
           contains(lower, "game_over");
}

PlayerSubtype defaultSubtypeFor(PlayerRole role) {
    if (role == PlayerRole::Pathogen) {
        return PathogenSubtype::Virus;
    }
    return HumanitySubtype::ResearchInstitute;
}

PlayerSubtype subtypeForIndex(PlayerRole role, int index) {
    (void)index;
    return defaultSubtypeFor(role);
}

const char* roleName(PlayerRole role) {
    return role == PlayerRole::Pathogen ? "Pathogen" : "Humanity";
}

const char* subtypeName(const PlayerSubtype& subtype) {
    if (std::holds_alternative<PathogenSubtype>(subtype)) {
        return "Virus";
    }
    return "ResearchInstitute";
}

const char* situationName(GameSituation situation) {
    switch (situation) {
    case GameSituation::MainMenu:
        return "MainMenu";
    case GameSituation::Settings:
        return "Settings";
    case GameSituation::ConnectToServer:
        return "ConnectToServer";
    case GameSituation::Exit:
        return "Exit";
    case GameSituation::ConnectingToServer:
        return "ConnectingToServer";
    case GameSituation::ConnectingToServerFailed:
        return "ConnectingToServerFailed";
    case GameSituation::ChoosingSide:
        return "ChoosingSide";
    case GameSituation::Game:
        return "Game";
    case GameSituation::EndScreen:
        return "EndScreen";
    }

    return "Unknown";
}

const char* flowStateName(ClientFlowState state) {
    switch (state) {
    case ClientFlowState::Disconnected:
        return "Disconnected";
    case ClientFlowState::Connecting:
        return "Connecting";
    case ClientFlowState::WaitingForRole:
        return "WaitingForRole";
    case ClientFlowState::ChoosingSubtype:
        return "ChoosingSubtype";
    case ClientFlowState::LobbyWaiting:
        return "LobbyWaiting";
    case ClientFlowState::ReadyWaitingStart:
        return "ReadyWaitingStart";
    case ClientFlowState::GameRunning:
        return "GameRunning";
    case ClientFlowState::GameOver:
        return "GameOver";
    }

    return "Unknown";
}

const char* choosingActionName(request::ChoosingSideAction action) {
    switch (action) {
    case request::ChoosingSideAction::SelectSubtype:
        return "SelectSubtype";
    case request::ChoosingSideAction::ChangeSide:
        return "ChangeSide";
    case request::ChoosingSideAction::Ready:
        return "Ready";
    }
    return "SelectSubtype";
}

ClientCommand commandForChoosingAction(request::ChoosingSideAction action) {
    switch (action) {
    case request::ChoosingSideAction::SelectSubtype:
        return ClientCommand::SelectSubtype;
    case request::ChoosingSideAction::ChangeSide:
        return ClientCommand::ChangeSide;
    case request::ChoosingSideAction::Ready:
        return ClientCommand::Ready;
    }
    return ClientCommand::SelectSubtype;
}

std::string makeChoosingSidePayload(request::ChoosingSideAction action,
                                    PlayerRole role,
                                    const PlayerSubtype& subtype) {
    std::ostringstream payload;
    payload << "action=" << choosingActionName(action)
            << ";role=" << roleName(role)
            << ";subtype=" << subtypeName(subtype);
    return payload.str();
}

void applyAssignedRole(GameState& gameState, PlayerRole role) {
    GameSnapshot snapshot = gameState.snapshot();
    snapshot.playerInfo.role = role;
    snapshot.playerInfo.subtype = defaultSubtypeFor(role);
    snapshot.choosingSide.predefinedRole = role;
    snapshot.choosingSide.selectedSubtype = snapshot.playerInfo.subtype;
    snapshot.choosingSide.signal = ChoosingSideSignal::None;

    gameState.setPlayerInfo(snapshot.playerInfo);
    gameState.setChoosingSideState(snapshot.choosingSide);
}

void applyChoosingSideSignal(GameState& gameState, const std::string& payload) {
    const std::string lower = toLower(payload);
    GameSnapshot snapshot = gameState.snapshot();
    ChoosingSideState choosing = snapshot.choosingSide;
    choosing.signal = ChoosingSideSignal::None;

    if (const auto ready = parseBoolField(payload, "ready")) {
        choosing.ready = *ready;
    }
    if (const auto sideChangeRequested = parseBoolField(payload, "sideChangeRequested")) {
        choosing.sideChangeRequested = *sideChangeRequested;
    }
    if (const auto opponentReady = parseBoolField(payload, "opponentReady")) {
        choosing.opponentReady = *opponentReady;
    }
    if (const auto opponentSideChangeRequested = parseBoolField(payload, "opponentSideChangeRequested")) {
        choosing.opponentSideChangeRequested = *opponentSideChangeRequested;
    }

    if (contains(lower, "\"event\":\"opponentready\"") ||
        contains(lower, "event=opponentready") ||
        contains(lower, "opponent_ready")) {
        choosing.signal = ChoosingSideSignal::OpponentReady;
    }
    if (contains(lower, "\"event\":\"opponentrequestssidechange\"") ||
        contains(lower, "event=opponentrequestssidechange") ||
        contains(lower, "opponent_side_change") ||
        contains(lower, "opponentrequestssidechange")) {
        choosing.signal = ChoosingSideSignal::OpponentRequestsSideChange;
    }

    gameState.setChoosingSideState(choosing);
}

}

ClientApp::ClientApp(SocketTransport& transport)
    : transport_(transport), request_handler_(std::make_unique<RequestHandler>(transport)) {
    request_handler_->setUnhandledResponseCallback([this](const ServerResponse& response) {
        handleServerPacket(response);
    });
}

void ClientApp::run() {
    LOG_INFO("Client event loop started in %s", situationName(game_state_.getSituation()));

    using clock = std::chrono::steady_clock;
    constexpr auto frameTime = std::chrono::microseconds(16667);
    auto nextFrame = clock::now();

    while (running_) {
        nextFrame += frameTime;

        const request::UIRequest request = renderer_.pollInput(game_state_);
        handleUserAction(request);
        request_handler_->update();
        renderer_.render(game_state_);

        std::this_thread::sleep_until(nextFrame);
        if (clock::now() > nextFrame + frameTime) {
            nextFrame = clock::now();
        }
    }
}

void ClientApp::setSituation(GameSituation newSituation) {
    const GameSituation oldSituation = game_state_.getSituation();
    if (oldSituation != newSituation) {
        LOG_INFO("Situation changed: %s -> %s",
                 situationName(oldSituation),
                 situationName(newSituation));
    }
    game_state_.setSituation(newSituation);
}

void ClientApp::resetStateForMenu() {
    LOG_INFO("Resetting client state and returning to main menu");
    game_state_.resetForMenu();
    std::lock_guard<std::mutex> lock(m_stateMutex);
    m_currentState = ClientFlowState::Disconnected;
    m_subtypeSelected = false;
}

void ClientApp::setFlowState(ClientFlowState newState) {
    std::lock_guard<std::mutex> lock(m_stateMutex);
    if (m_currentState != newState) {
        LOG_INFO("Client flow changed: %s -> %s",
                 flowStateName(m_currentState),
                 flowStateName(newState));
    }
    m_currentState = newState;
}

ClientFlowState ClientApp::getFlowState() const {
    std::lock_guard<std::mutex> lock(m_stateMutex);
    return m_currentState;
}

void ClientApp::handleServerPacket(const Packet& packet) {
    LOG_DEBUG("Received server packet: request_id=%u success=%d payload=%s error=%s",
              packet.request_id,
              packet.success ? 1 : 0,
              packet.payload.c_str(),
              packet.error_message.c_str());

    if (!packet.success) {
        LOG_WARNING("Server rejected request %u: %s",
                    packet.request_id,
                    packet.error_message.c_str());
        transport_.disconnect();
        resetStateForMenu();
        setSituation(GameSituation::MainMenu);
        return;
    }

    if (const auto role = parseRoleFromPayload(packet.payload)) {
        LOG_INFO("Assigned role: %s", roleName(*role));
        applyAssignedRole(game_state_, *role);
        std::lock_guard<std::mutex> lock(m_stateMutex);
        m_subtypeSelected = true;
    }
    applyChoosingSideSignal(game_state_, packet.payload);

    if (packetEndsGame(packet.payload)) {
        LOG_INFO("Server ended the game");
        setFlowState(ClientFlowState::GameOver);
        setSituation(GameSituation::EndScreen);
        return;
    }

    if (packetStartsGame(packet.payload)) {
        LOG_INFO("Game start packet received");
        if (const auto day = parseIntField(packet.payload, "day")) {
            game_state_.setDay(static_cast<std::uint16_t>(std::max(0, *day)));
        } else {
            game_state_.setDay(1);
        }

        if (const auto points = parseIntField(packet.payload, "points")) {
            game_state_.setPlayerPoints(static_cast<UpgradePointType>(std::max(0, *points)));
        }

        setFlowState(ClientFlowState::GameRunning);
        setSituation(GameSituation::Game);
        return;
    }

    if (packetShowsGame(packet.payload)) {
        if (const auto day = parseIntField(packet.payload, "day")) {
            game_state_.setDay(static_cast<std::uint16_t>(std::max(0, *day)));
        }

        if (const auto points = parseIntField(packet.payload, "points")) {
            game_state_.setPlayerPoints(static_cast<UpgradePointType>(std::max(0, *points)));
        }

        LOG_DEBUG("Game update packet received");
        return;
    }

    switch (getFlowState()) {
    case ClientFlowState::WaitingForRole:
        if (packetShowsChoosingSide(packet.payload) || parseRoleFromPayload(packet.payload).has_value()) {
            setFlowState(ClientFlowState::ChoosingSubtype);
            setSituation(GameSituation::ChoosingSide);
        }
        break;

    case ClientFlowState::ChoosingSubtype:
    case ClientFlowState::LobbyWaiting:
    case ClientFlowState::ReadyWaitingStart:
        if (packetShowsChoosingSide(packet.payload)) {
            setSituation(GameSituation::ChoosingSide);
        }
        break;

    case ClientFlowState::Disconnected:
    case ClientFlowState::Connecting:
    case ClientFlowState::GameRunning:
    case ClientFlowState::GameOver:
        break;
    }
}

void ClientApp::handleUserAction(const UserAction& request) {
    switch (game_state_.getSituation()) {
    case GameSituation::MainMenu:
        if (std::holds_alternative<request::MainMenu>(request) &&
            std::get<request::MainMenu>(request) == request::MainMenu::ConnectToServer) {
            LOG_INFO("Main menu action: connect to server");
            setFlowState(ClientFlowState::Connecting);
            setSituation(GameSituation::ConnectingToServer);
        } else if (std::holds_alternative<request::MainMenu>(request) &&
                   std::get<request::MainMenu>(request) == request::MainMenu::OpenSettings) {
            LOG_INFO("Main menu action: open settings");
            setSituation(GameSituation::Settings);
        } else if (std::holds_alternative<request::MainMenu>(request) &&
                   std::get<request::MainMenu>(request) == request::MainMenu::Exit) {
            LOG_INFO("Main menu action: exit client");
            setSituation(GameSituation::Exiting);
            running_ = false;
        }
        break;

    case GameSituation::Settings:
        if (std::holds_alternative<request::Settings>(request) &&
            std::get<request::Settings>(request) == request::Settings::Back) {
            LOG_INFO("Settings action: back to main menu");
            setSituation(GameSituation::MainMenu);
        }
        break;

    case GameSituation::ConnectToServer:
    case GameSituation::ConnectingToServer:
        if (std::holds_alternative<request::ConnectInfo>(request) &&
            std::get<request::ConnectInfo>(request).id == request::Connect::Connect) {
            if (request_handler_->hasPendingRequests()) {
                LOG_DEBUG("Connect action ignored because a request is already pending");
                break;
            }

            const auto& info = std::get<request::ConnectInfo>(request);
            std::string host = info.addr.empty() ? kServerHost : info.addr;
            int port = kServerPort;
            if (!info.port.empty()) {
                try {
                    port = std::stoi(info.port);
                } catch (...) {}
            }

            LOG_INFO("Connect form submitted: host=%s port=%d", host.c_str(), port);

            if (!transport_.isConnected() && !transport_.connectToServer(host.c_str(), port)) {
                LOG_WARNING("Connection attempt failed: host=%s port=%d", host.c_str(), port);
                resetStateForMenu();
                setSituation(GameSituation::MainMenu);
                break;
            }

            LOG_INFO("Connection established, requesting lobby role");
            setFlowState(ClientFlowState::WaitingForRole);
            request_handler_->sendRequest(
                ClientCommand::Connect,
                "",
                [this](const ServerResponse& response) {
                    handleServerPacket(response);
                },
                [this](RequestId) {
                    LOG_WARNING("Connect request timed out");
                    transport_.disconnect();
                    resetStateForMenu();
                    setSituation(GameSituation::MainMenu);
                });
        } else if (std::holds_alternative<request::ConnectInfo>(request) &&
                   std::get<request::ConnectInfo>(request).id == request::Connect::Back) {
            LOG_INFO("Connect screen action: back to main menu");
            setSituation(GameSituation::MainMenu);
        }
        break;

    case GameSituation::ConnectingToServerFailed:
        resetStateForMenu();
        setSituation(GameSituation::MainMenu);
        break;

    case GameSituation::ChoosingSide:
        if (request_handler_->hasPendingRequests()) {
            LOG_DEBUG("Choosing-side action ignored because a request is already pending");
            break;
        }

        if (std::holds_alternative<request::ChoosingSide>(request)) {
            const request::ChoosingSide choosingRequest = std::get<request::ChoosingSide>(request);
            const GameSnapshot snapshot = game_state_.snapshot();
            const PlayerRole role = snapshot.playerInfo.role;
            PlayerSubtype subtype = snapshot.choosingSide.selectedSubtype;
            ChoosingSideState choosingState = snapshot.choosingSide;
            bool shouldSendRequest = true;

            switch (choosingRequest.action) {
            case request::ChoosingSideAction::SelectSubtype: {
                if (getFlowState() == ClientFlowState::ReadyWaitingStart) {
                    shouldSendRequest = false;
                    break;
                }

                subtype = subtypeForIndex(role, choosingRequest.subtypeIndex);
                LOG_INFO("Choosing-side action: select subtype=%s role=%s",
                         subtypeName(subtype),
                         roleName(role));
                InfoAboutPlayer playerInfo = snapshot.playerInfo;
                playerInfo.subtype = subtype;
                choosingState.selectedSubtype = subtype;
                choosingState.ready = false;
                choosingState.signal = ChoosingSideSignal::None;

                game_state_.setPlayerInfo(playerInfo);
                game_state_.setChoosingSideState(choosingState);

                {
                    std::lock_guard<std::mutex> lock(m_stateMutex);
                    m_subtypeSelected = true;
                    m_currentState = ClientFlowState::LobbyWaiting;
                }
                break;
            }

            case request::ChoosingSideAction::ChangeSide:
                if (getFlowState() == ClientFlowState::ReadyWaitingStart) {
                    shouldSendRequest = false;
                    break;
                }

                choosingState.sideChangeRequested = true;
                LOG_INFO("Choosing-side action: request side change");
                choosingState.signal = ChoosingSideSignal::None;
                game_state_.setChoosingSideState(choosingState);
                setFlowState(ClientFlowState::LobbyWaiting);
                break;

            case request::ChoosingSideAction::Ready: {
                bool subtypeSelected = false;
                {
                    std::lock_guard<std::mutex> lock(m_stateMutex);
                    subtypeSelected = m_subtypeSelected;
                }

                if (!subtypeSelected) {
                    LOG_WARNING("Ready action rejected locally: subtype is not selected");
                    shouldSendRequest = false;
                    break;
                }

                choosingState.ready = true;
                LOG_INFO("Choosing-side action: ready");
                choosingState.signal = ChoosingSideSignal::LocalReady;
                game_state_.setChoosingSideState(choosingState);
                setFlowState(ClientFlowState::ReadyWaitingStart);
                break;
            }
            }

            if (!shouldSendRequest) {
                break;
            }

            request_handler_->sendRequest(
                commandForChoosingAction(choosingRequest.action),
                makeChoosingSidePayload(choosingRequest.action, role, subtype),
                [this](const ServerResponse& response) {
                    handleServerPacket(response);
                },
                [this](RequestId) {
                    LOG_WARNING("Lobby action request timed out");
                    if (getFlowState() == ClientFlowState::ReadyWaitingStart) {
                        setFlowState(ClientFlowState::LobbyWaiting);
                    }
                });
        } else if (std::holds_alternative<request::MainMenu>(request) &&
                   std::get<request::MainMenu>(request) == request::MainMenu::Exit) {
            transport_.disconnect();
            LOG_INFO("Choosing-side action: exit to main menu");
            resetStateForMenu();
            setSituation(GameSituation::MainMenu);
        }
        break;

    case GameSituation::Game:
        if (std::holds_alternative<request::Settings>(request) &&
            std::get<request::Settings>(request) == request::Settings::Back) {
            LOG_INFO("Game action: leave current game");
            setFlowState(ClientFlowState::GameOver);
            setSituation(GameSituation::EndScreen);
        }
        break;

    case GameSituation::EndScreen:
        if (std::holds_alternative<request::Settings>(request) &&
            std::get<request::Settings>(request) == request::Settings::Back) {
            LOG_INFO("End screen action: back to main menu");
            transport_.disconnect();
            resetStateForMenu();
            setSituation(GameSituation::MainMenu);
        }
        break;

    case GameSituation::Exiting:
        running_ = false;
        break;
    }
}

}
