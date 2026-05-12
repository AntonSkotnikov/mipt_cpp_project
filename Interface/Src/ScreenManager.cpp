#include "ScreenManager.hpp"
#include "Screen.hpp"
#include "Settings.hpp"
#include "Window.hpp"
#include <ncurses.h>

namespace plague::ui {

namespace {

Screen * screenById(ScreenManager & manager, ScreenIds id) {
    switch (id) {
        case ScreenIds::SmallTerm: return &manager.smallTerm_;
        case ScreenIds::MainMenu:  return &manager.mainMenu_;
        case ScreenIds::Connect:   return &manager.connect_;
        case ScreenIds::Settings:  return &manager.mainMenu_;
        case ScreenIds::Game:      return &manager.game_;
        case ScreenIds::Info:      return &manager.pathogen_;
        case ScreenIds::Transmission: return &manager.transmission_;
        case ScreenIds::Clinic:    return &manager.clinic_;
        case ScreenIds::Abilities: return &manager.abilities_;
        case ScreenIds::Cure:      return &manager.cure_;
        case ScreenIds::World:     return &manager.country_;
        case ScreenIds::News:      return &manager.news_;
    }

    return manager.curScreen;
}

}

ScreenManager::ScreenManager(Config & cfg) : cfg_(cfg),
                                             mainWin_{terminalProfiles.at(cfg.resolution).height + deltaForBorders, terminalProfiles.at(cfg.resolution).width + deltaForBorders},
                                             smallTerm_{cfg, mainWin_},
                                             mainMenu_{cfg, mainWin_},
                                             connect_(cfg, mainWin_),
                                             choosingSide_(cfg, mainWin_),
                                             game_{cfg, mainWin_},
                                             pathogen_{cfg, mainWin_},
                                             transmission_{cfg, mainWin_, UpgradeCategory::Transmission},
                                             clinic_{cfg, mainWin_, UpgradeCategory::Clinic},
                                             abilities_{cfg, mainWin_, UpgradeCategory::Abilities},
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
    transmission_.resize();
    clinic_.resize();
    abilities_.resize();
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

    curScreen = screenById(*this, cfg_.id);
}

}
