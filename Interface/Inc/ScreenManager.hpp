#pragma once

#include "Screen.hpp"
#include "Settings.hpp"
#include "Window.hpp"

namespace plague::ui {

class ScreenManager final {
private:
    Config & cfg_;
    Window mainWin_; // for border if needed

    SmallTermScreen smallTerm_;
    MainMenuScreen mainMenu_;
    ConnectToServerScreen connect_;
    //Screen settings_;
    //Screen choosingSide;
    //Screen game_;
public:
    Screen * curScreen = &mainMenu_;
    ScreenManager(Config & cfg);
    void resize();
private:
    void applyWindowLayout();
    void selectCurrentScreen();
};

}
