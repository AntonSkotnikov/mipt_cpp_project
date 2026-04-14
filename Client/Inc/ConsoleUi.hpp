#pragma once

#include "IUserInterface.hpp"

namespace plague {

class ConsoleUi final : public IUserInterface {
public:
    void render(GameSituation situation) override;
    ClientEvent pollEvent(GameSituation situation) override;
    void showMessage(const char* text) override;
};

}
