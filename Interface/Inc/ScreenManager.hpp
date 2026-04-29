#pragma once

#include "Screen.hpp"
#include "Settings.hpp"
#include "Window.hpp"

namespace plague::ui {

class ScreenManager final {
private:
    Window mainWin_; // for border 
    MainMenuScreen mainMenu_;
    //Screen connect_;
    //Screen settings_;
    //Screen choosingSide;
    //Screen game_;
public:
    Screen * curScreen = &mainMenu_;
    ScreenManager(Config & cfg);
};

}