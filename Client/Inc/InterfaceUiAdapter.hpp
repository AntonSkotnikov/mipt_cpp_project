#pragma once

#include "IUserInterface.hpp"

#include <string>

namespace plague {

class InterfaceUiAdapter final : public IUserInterface {
public:
    InterfaceUiAdapter();
    ~InterfaceUiAdapter() override;

    void render(const GameSnapshot& snapshot) override;
    request::UIRequest pollRequest(const GameSnapshot& snapshot) override;
    void showMessage(const char* text) override;

private:
    void renderScreen(const GameSnapshot& snapshot,
                      const char* title,
                      const char* option1 = nullptr,
                      const char* option2 = nullptr,
                      const char* option3 = nullptr) const;

private:
    std::string last_message_{};
};

}
