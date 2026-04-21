#pragma once

#include "GameTypes.hpp"
#include "ClientEvents.hpp"
#include "ITransport.hpp"
#include "IUserInterface.hpp"
#include "RequestHandler.hpp"
#include <memory>

namespace plague {

class ClientApp {
public:
    ClientApp(IUserInterface& ui, ITransport& transport);
    void run();

private:
    void handleEvent(ClientEvent event);
    void setSituation(GameSituation newSituation);

private:
    IUserInterface& ui_;
    ITransport& transport_;
    std::unique_ptr<RequestHandler> request_handler_;
    GameSituation situation_ = GameSituation::MainMenu;
    bool running_ = true;
};

}
