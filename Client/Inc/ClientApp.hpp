#pragma once

#include "ITransport.hpp"
#include "IUserInterface.hpp"
#include "RequestHandler.hpp"
#include "UI_ClientAPI.hpp"

#include <memory>

namespace plague {

class ClientApp {
public:
    ClientApp(IUserInterface& ui, ITransport& transport);
    void run();

private:
    void handleRequest(const request::UIRequest& request);
    void setSituation(GameSituation newSituation);
    void resetSnapshotForMenu();

private:
    IUserInterface& ui_;
    ITransport& transport_;
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
