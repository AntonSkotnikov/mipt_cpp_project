#pragma once

#include "GameTypes.hpp"
#include "ScreenManager.hpp"
#include "Settings.hpp"
#include "UIRequest.hpp"
#include <memory>

namespace plague::ui {

class UIManager final {
private:
    std::unique_ptr<ScreenManager> man_;
    Config        cfg_;
    GameSituation snap_;

public:
    UIManager(); // init of ncurses etc
    ~UIManager();
    request::UIRequest loop();
};

}