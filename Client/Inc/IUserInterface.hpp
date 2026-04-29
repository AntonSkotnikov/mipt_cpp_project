#pragma once

#include "UI_ClientAPI.hpp"
#include "UIRequest.hpp"

namespace plague {

class IUserInterface {
public:
    virtual ~IUserInterface() = default;

    virtual void render(const GameSnapshot& snapshot) = 0;
    virtual request::UIRequest pollRequest(const GameSnapshot& snapshot) = 0;
    virtual void showMessage(const char* text) = 0;
};

}
