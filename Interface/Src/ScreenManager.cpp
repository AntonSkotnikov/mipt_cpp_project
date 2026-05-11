#include "ScreenManager.hpp"
#include "Screen.hpp"
#include "Settings.hpp"
#include "Window.hpp"
#include <ncurses.h>

namespace plague::ui {

ScreenManager::ScreenManager(Config & cfg) : cfg_(cfg),
                                             mainWin_{terminalProfiles.at(cfg.resolution).height + deltaForBorders, terminalProfiles.at(cfg.resolution).width + deltaForBorders},
                                             smallTerm_{cfg, mainWin_},
                                             mainMenu_{cfg, mainWin_},
                                             connect_(cfg, mainWin_),
                                             choosingSide_(cfg, mainWin_),
                                             game_{cfg, mainWin_},
                                             pathogen_{cfg, mainWin_},
                                             cure_{cfg, mainWin_},
                                             country_{cfg, mainWin_},
                                             news_{cfg, mainWin_},
                                             curScreen(&mainMenu_) {
    selectCurrentScreen();
    applyWindowLayout();
     curScreen->resize();
}

void ScreenManager::resize() {
    selectCurrentScreen();
    applyWindowLayout();
    smallTerm_.resize();
    mainMenu_.resize();
    connect_.resize();
    choosingSide_.resize();
    game_.resize();
    pathogen_.resize();
    cure_.resize();
    country_.resize();
    news_.resize();
}

void ScreenManager::applyWindowLayout() {
    int terminalHeight = 0;
    int terminalWidth = 0;
    getmaxyx(stdscr, terminalHeight, terminalWidth);

    if (cfg_.terminalTooSmall) {
        mainWin_.setBorders(false);
        mainWin_.resizeCentered(terminalHeight, terminalWidth);
        return;
    }

    const TerminalProfile profile = terminalProfiles.at(cfg_.resolution);
    const bool profileMatchesTerminal = terminalHeight == profile.height && terminalWidth == profile.width;
    const bool bordered = !profileMatchesTerminal;
    mainWin_.setBorders(bordered);

    const int targetHeight = profile.height + (bordered ? deltaForBorders : 0);
    const int targetWidth = profile.width + (bordered ? deltaForBorders : 0);
    mainWin_.resizeCentered(targetHeight, targetWidth);
}

void ScreenManager::selectCurrentScreen() {
    if (cfg_.terminalTooSmall) {
        curScreen = &smallTerm_;
        return;
    }

    switch (cfg_.id) {
        case ScreenIds::SmallTerm:
            curScreen = &smallTerm_;
            return;
        case ScreenIds::MainMenu:
            curScreen = &mainMenu_;
            return;
        case ScreenIds::Connect:
            curScreen = &connect_;
            return;
        case ScreenIds::Settings:
            curScreen = &mainMenu_;
            return;
        case ScreenIds::Game:
            curScreen = &game_;
            return;
        case ScreenIds::Info:
            curScreen = &pathogen_;
            return;
        case ScreenIds::Cure:
            curScreen = &cure_;
            return;
        case ScreenIds::World:
            curScreen = &country_;
            return;
        case ScreenIds::News:
            curScreen = &news_;
            return;
        case ScreenIds::Transmission:
        case ScreenIds::Clinic:
        case ScreenIds::Abilities:
            return;
    }
}

}
