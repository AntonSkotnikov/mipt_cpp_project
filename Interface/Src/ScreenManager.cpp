#include "ScreenManager.hpp"
#include "Settings.hpp"
#include "Window.hpp"

namespace plague::ui {

ScreenManager::ScreenManager(Config & cfg) : mainWin_{terminalProfiles.at(cfg.resolution).height + deltaForBorders, terminalProfiles.at(cfg.resolution).width + deltaForBorders},
                                             mainMenu_{cfg, mainWin_},
                                             curScreen(&mainMenu_) {}

}