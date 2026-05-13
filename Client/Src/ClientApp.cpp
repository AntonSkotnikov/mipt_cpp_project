#include "ClientApp.hpp"

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

bool packetShowsChoosingSide(const std::string& payload) {
    const std::string lower = toLower(payload);
    return contains(lower, "choosingside") ||
           contains(lower, "choosing_side") ||
           contains(lower, "lobby");
}

bool packetStartsGame(const std::string& payload) {
    const std::string lower = toLower(payload);
    return contains(lower, "\"screen\":\"game\"") ||
           contains(lower, "screen=game") ||
           contains(lower, "gamestart") ||
           contains(lower, "game_start") ||
           (contains(lower, "start") && contains(lower, "game"));
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

ClientCommand commandForChoosingAction(request::ChoosingSideAction action, PlayerRole role) {
    if (action != request::ChoosingSideAction::Ready) {
        return ClientCommand::Ping;
    }
    return role == PlayerRole::Pathogen ? ClientCommand::ChoosePathogen : ClientCommand::ChooseHumanity;
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

    if (contains(lower, "opponentready") || contains(lower, "opponent_ready")) {
        choosing.opponentReady = true;
        choosing.signal = ChoosingSideSignal::OpponentReady;
    }
    if (contains(lower, "opponentsidechangerequested") ||
        contains(lower, "opponent_side_change") ||
        contains(lower, "opponentrequestssidechange")) {
        choosing.opponentSideChangeRequested = true;
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
    game_state_.setSituation(newSituation);
}

void ClientApp::resetStateForMenu() {
    game_state_.resetForMenu();
    std::lock_guard<std::mutex> lock(m_stateMutex);
    m_currentState = ClientFlowState::Disconnected;
    m_subtypeSelected = false;
}

void ClientApp::setFlowState(ClientFlowState newState) {
    std::lock_guard<std::mutex> lock(m_stateMutex);
    m_currentState = newState;
}

ClientFlowState ClientApp::getFlowState() const {
    std::lock_guard<std::mutex> lock(m_stateMutex);
    return m_currentState;
}

void ClientApp::handleServerPacket(const Packet& packet) {
    if (!packet.success) {
        transport_.disconnect();
        resetStateForMenu();
        setSituation(GameSituation::MainMenu);
        return;
    }

    if (const auto role = parseRoleFromPayload(packet.payload)) {
        applyAssignedRole(game_state_, *role);
        std::lock_guard<std::mutex> lock(m_stateMutex);
        m_subtypeSelected = true;
    }
    applyChoosingSideSignal(game_state_, packet.payload);

    if (packetEndsGame(packet.payload)) {
        setFlowState(ClientFlowState::GameOver);
        setSituation(GameSituation::EndScreen);
        return;
    }

    if (packetStartsGame(packet.payload)) {
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
            setFlowState(ClientFlowState::Connecting);
            setSituation(GameSituation::ConnectingToServer);
        } else if (std::holds_alternative<request::MainMenu>(request) &&
                   std::get<request::MainMenu>(request) == request::MainMenu::OpenSettings) {
            setSituation(GameSituation::Settings);
        } else if (std::holds_alternative<request::MainMenu>(request) &&
                   std::get<request::MainMenu>(request) == request::MainMenu::Exit) {
            setSituation(GameSituation::Exiting);
            running_ = false;
        }
        break;

    case GameSituation::Settings:
        if (std::holds_alternative<request::Settings>(request) &&
            std::get<request::Settings>(request) == request::Settings::Back) {
            setSituation(GameSituation::MainMenu);
        }
        break;

    case GameSituation::ConnectToServer:
    case GameSituation::ConnectingToServer:
        if (std::holds_alternative<request::ConnectInfo>(request) &&
            std::get<request::ConnectInfo>(request).id == request::Connect::Connect) {
            if (request_handler_->hasPendingRequests()) {
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

            if (!transport_.isConnected() && !transport_.connectToServer(host.c_str(), port)) {
                resetStateForMenu();
                setSituation(GameSituation::MainMenu);
                break;
            }

            setFlowState(ClientFlowState::WaitingForRole);
            request_handler_->sendRequest(
                ClientCommand::Connect,
                "",
                [this](const ServerResponse& response) {
                    game_state_.clearNews();
                    handleServerPacket(response);
                },
                [this](RequestId) {
                    transport_.disconnect();
                    resetStateForMenu();
                    setSituation(GameSituation::MainMenu);
                });
        } else if (std::holds_alternative<request::ConnectInfo>(request) &&
                   std::get<request::ConnectInfo>(request).id == request::Connect::Back) {
            setSituation(GameSituation::MainMenu);
        }
        break;

    case GameSituation::ConnectingToServerFailed:
        resetStateForMenu();
        setSituation(GameSituation::MainMenu);
        break;

    case GameSituation::ChoosingSide:
        if (request_handler_->hasPendingRequests()) {
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
                    game_state_.addNews(ImportanceOfNews::RegularNews, "Choose a subtype before pressing Ready.");
                    shouldSendRequest = false;
                    break;
                }

                choosingState.ready = true;
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
                commandForChoosingAction(choosingRequest.action, role),
                makeChoosingSidePayload(choosingRequest.action, role, subtype),
                [this](const ServerResponse& response) {
                    handleServerPacket(response);
                },
                [this](RequestId) {
                    if (getFlowState() == ClientFlowState::ReadyWaitingStart) {
                        setFlowState(ClientFlowState::LobbyWaiting);
                    }
                    game_state_.addNews(ImportanceOfNews::RegularNews, "Server did not confirm lobby action.");
                });
        } else if (std::holds_alternative<request::MainMenu>(request) &&
                   std::get<request::MainMenu>(request) == request::MainMenu::Exit) {
            transport_.disconnect();
            resetStateForMenu();
            setSituation(GameSituation::MainMenu);
        }
        break;

    case GameSituation::Game:
        if (std::holds_alternative<request::Settings>(request) &&
            std::get<request::Settings>(request) == request::Settings::Back) {
            game_state_.addNews(ImportanceOfNews::RegularNews, "Left current game.");
            setFlowState(ClientFlowState::GameOver);
            setSituation(GameSituation::EndScreen);
        }
        break;

    case GameSituation::EndScreen:
        if (std::holds_alternative<request::Settings>(request) &&
            std::get<request::Settings>(request) == request::Settings::Back) {
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
