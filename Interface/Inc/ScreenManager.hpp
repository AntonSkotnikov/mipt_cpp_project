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

    GameScreen game_;
    ////InfoScreen info_;
    //TransmissionScreen trans_; // carantine for humanity
    //ClinicScreen clinic_;
    //AbilitiesScreen abilities_; // operation for humanity
//
    //WorldScreen world_;
    //CureScreen cure_;
    //NewsScreen news_;

    Screen * curScreen = &mainMenu_;
    ScreenManager(Config & cfg);
    void resize();
private:
    void applyWindowLayout();
    void selectCurrentScreen();
};

}
