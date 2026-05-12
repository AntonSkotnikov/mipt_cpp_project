#pragma once

#include "GameRenderer.hpp"
#include "GameState.hpp"
#include "RequestHandler.hpp"
#include "SocketTransport.hpp"
#include "UI_ClientAPI.hpp"

#include <memory>
#include <mutex>

namespace plague {

using Packet = ServerResponse;
using UserAction = request::UIRequest;

enum class ClientFlowState {
    Disconnected,
    Connecting,
    WaitingForRole,
    ChoosingSubtype,
    LobbyWaiting,
    ReadyWaitingStart,
    GameRunning,
    GameOver
};

class ClientApp {
public:
    explicit ClientApp(SocketTransport& transport);
    void run();

private:
    void handleServerPacket(const Packet& packet);
    void handleUserAction(const UserAction& action);
    void setSituation(GameSituation newSituation);
    void resetStateForMenu();
    void setFlowState(ClientFlowState newState);
    ClientFlowState getFlowState() const;

private:
    SocketTransport& transport_;
    std::unique_ptr<RequestHandler> request_handler_;
    GameState game_state_;
    GameRenderer renderer_;
    mutable std::mutex m_stateMutex;
    ClientFlowState m_currentState = ClientFlowState::Disconnected;
    bool m_subtypeSelected = false;
    bool running_ = true;
};

}
