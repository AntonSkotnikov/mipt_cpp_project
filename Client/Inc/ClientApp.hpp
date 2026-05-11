#pragma once

#include "GameRenderer.hpp"
#include "GameState.hpp"
#include "RequestHandler.hpp"
#include "SocketTransport.hpp"
#include "UI_ClientAPI.hpp"

#include <memory>

namespace plague {

class ClientApp {
public:
    explicit ClientApp(SocketTransport& transport);
    void run();

private:
    void handleRequest(const request::UIRequest& request);
    void setSituation(GameSituation newSituation);
    void resetStateForMenu();

private:
    SocketTransport& transport_;
    std::unique_ptr<RequestHandler> request_handler_;
    GameState game_state_;
    GameRenderer renderer_;
    bool running_ = true;
};

}
