#pragma once

#include "GameRenderer.hpp"
#include "GameState.hpp"
#include "RequestHandler.hpp"
#include "SocketTransport.hpp"
#include "UI_ClientAPI.hpp"

#include <future>
#include <memory>
#include <mutex>
#include <utility>

namespace plague {

using Packet = ServerResponse;
using UserAction = request::UIRequest;

// Внутренний state machine клиента поверх экранов UI.
enum class ClientFlowState {
    Disconnected,
    Connecting,
    RoomBrowsing,
    WaitingForRole,
    ChoosingSubtype,
    LobbyWaiting,
    ReadyWaitingStart,
    GameRunning,
    GameOver
};

// Главный цикл клиента: читает UI, шлёт команды серверу и обновляет GameState.
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
    void updatePendingConnection();

private:
    SocketTransport& transport_;
    std::unique_ptr<RequestHandler> request_handler_;
    std::future<std::pair<int, bool>> pending_connection_;
    GameState game_state_;
    GameRenderer renderer_;
    mutable std::mutex m_stateMutex;
    ClientFlowState m_currentState = ClientFlowState::Disconnected;
    int m_nextConnectionAttemptId = 1;
    int m_activeConnectionAttemptId = 0;
    bool m_subtypeSelected = false;
    bool running_ = true;
};

}
