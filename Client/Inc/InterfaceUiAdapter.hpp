#pragma once

#include "IUserInterface.hpp"
#include "UIManager.hpp"

#include <memory>
#include <string>

namespace plague {

class InterfaceUiAdapter final : public IUserInterface {
public:
    InterfaceUiAdapter();
    ~InterfaceUiAdapter() override = default;

    void render(const GameSnapshot& snapshot) override;
    request::UIRequest pollRequest(const GameSnapshot& snapshot) override;
    void showMessage(const char* text) override;

private:
    void renderScreen(const GameSnapshot& snapshot,
                      const char* title,
                      const char* option1 = nullptr,
                      const char* option2 = nullptr,
                      const char* option3 = nullptr) const;
    bool usesInterfaceLoop(const GameSnapshot& snapshot) const;

private:
    std::unique_ptr<ui::UIManager> manager_;
    std::string last_message_{};
};

}
