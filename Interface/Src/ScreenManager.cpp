#include "ScreenManager.hpp"
#include "Settings.hpp"
#include "Window.hpp"
#include <ncurses.h>

namespace plague::ui {

ScreenManager::ScreenManager(Config & cfg) : cfg_(cfg),
                                             mainWin_{terminalProfiles.at(cfg.resolution).height + deltaForBorders, terminalProfiles.at(cfg.resolution).width + deltaForBorders},
                                             mainMenu_{cfg, mainWin_},
                                             curScreen(&mainMenu_) {
    applyWindowLayout();
}

void ScreenManager::resize() {
    applyWindowLayout();
    curScreen->resize();
}

void ScreenManager::applyWindowLayout() {
    const TerminalProfile profile = terminalProfiles.at(cfg_.resolution);

    int terminalHeight = 0;
    int terminalWidth = 0;
    getmaxyx(stdscr, terminalHeight, terminalWidth);

    const bool profileMatchesTerminal = terminalHeight == profile.height && terminalWidth == profile.width;
    const bool bordered = !profileMatchesTerminal;
    mainWin_.setBorders(bordered);

    const int targetHeight = profile.height + (bordered ? deltaForBorders : 0);
    const int targetWidth = profile.width + (bordered ? deltaForBorders : 0);
    mainWin_.resizeCentered(targetHeight, targetWidth);
}

}
