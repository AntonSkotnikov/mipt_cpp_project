#pragma once

#include "Screen.hpp"
#include "Settings.hpp"
#include "Window.hpp"

namespace plague::ui {

class ScreenManager final {
private:
    Config & cfg_;
    Window mainWin_; // for border if needed
public:
    SmallTermScreen smallTerm_;
    MainMenuScreen mainMenu_;
    ConnectToServerScreen connect_;
    RoomBrowserScreen rooms_;
    ChoosingSideScreen choosingSide_;

    GameScreen game_;

    PathogenInfoScreen pathogen_;
    UpgradeScreen transmission_;
    UpgradeScreen clinic_;
    UpgradeScreen abilities_;
    CureInfoScreen cure_;
    CountryScreen country_;
    NewsScreen news_;

    Screen * curScreen = &mainMenu_;
    ScreenManager(Config & cfg);
    void resize();
private:
    void applyWindowLayout();
    void selectCurrentScreen();
};

}
