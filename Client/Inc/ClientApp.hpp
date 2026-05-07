#pragma once

#include "SocketTransport.hpp"
#include "UIManager.hpp"
#include "RequestHandler.hpp"
#include "UI_ClientAPI.hpp"

#include <memory>

namespace plague {

class ClientApp {
public:
    ClientApp(ui::UIManager& ui, SocketTransport& transport);
    void run();

private:
    void handleRequest(const request::UIRequest& request);
    void setSituation(GameSituation newSituation);
    void resetSnapshotForMenu();

private:
    ui::UIManager& ui_;
    SocketTransport& transport_;
    std::unique_ptr<RequestHandler> request_handler_;
    GameSnapshot snapshot_{
        GameSituation::MainMenu,
        0,
        InfoAboutPlayer{PlayerRole::Humanity, 0},
        {}
    };
    bool running_ = true;
};

}
