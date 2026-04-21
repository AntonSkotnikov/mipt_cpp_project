#pragma once

#include <optional>
#include <variant>

#include "GameTypes.hpp"
#include "MainMenu.hpp"
#include "Settings.hpp"
#include "UIRequest.hpp"

namespace plague::ui {

class UiManager {
public:
    UiManager() = default;
    ~UiManager() = default;

    void init();
    void shutdown();
    void setSituation(plague::GameSituation situation);

    void draw();

    std::optional<plague::request::UIRequest> pollRequest();

private:
    plague::GameSituation situation_ = plague::GameSituation::MainMenu;

    MainMenuScreen mainMenuScreen_{};
    SettingsScreen settingsScreen_{};

    Screen& currentScreen();
};

}  // namespace plague::ui
