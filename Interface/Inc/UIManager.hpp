#pragma once

#include "GameTypes.hpp"
#include "ScreenManager.hpp"
#include "Settings.hpp"

namespace plague::ui {

class UIManager final {
private:
    ScreenManager man_;
    Config        cfg_;
    GameSituation snap_;

public:
    UIManager(); // init of ncurses etc
};

}