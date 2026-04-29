#pragma once

#include "GameTypes.hpp"
#include "UIRequest.hpp"

namespace plague {

class IUserInterface {
public:
    virtual ~IUserInterface() = default;

    virtual void render(GameSituation situation) = 0;
    virtual request::UIRequest pollRequest(GameSituation situation) = 0;
    virtual void showMessage(const char* text) = 0;
};

}
