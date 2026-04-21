#pragma once

#include "IUserInterface.hpp"
#include "UIManager.hpp"

#include <string>

namespace plague {

class InterfaceUiAdapter final : public IUserInterface {
public:
    InterfaceUiAdapter();
    ~InterfaceUiAdapter() override;

    void render(GameSituation situation) override;
    request::UIRequest pollRequest(GameSituation situation) override;
    void showMessage(const char* text) override;

private:
    bool isManagedByUiManager(GameSituation situation) const;
    void renderFallback(GameSituation situation) const;
    request::UIRequest pollFallbackRequest(GameSituation situation) const;

private:
    ui::UiManager manager_{};
    std::string last_message_{};
};

}
