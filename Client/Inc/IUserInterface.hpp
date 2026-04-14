#pragma once

#include "GameTypes.hpp"
#include "ClientEvents.hpp"

namespace plague {

class IUserInterface {
public:
    virtual ~IUserInterface() = default;

    virtual void render(GameSituation situation) = 0;
    virtual ClientEvent pollEvent(GameSituation situation) = 0;
    virtual void showMessage(const char* text) = 0;
};

}
