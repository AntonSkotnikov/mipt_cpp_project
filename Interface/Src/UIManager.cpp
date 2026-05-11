#include "UIManager.hpp"
#include "GameTypes.hpp"
#include "ScreenManager.hpp"
#include "Settings.hpp"
#include "UIRequest.hpp"
#include "UI_ClientAPI.hpp"
#include <clocale>
#include <cstdlib>
#include <stdexcept>
#include <string_view>
#include <ncurses.h>

namespace plague::ui {

namespace {

Resolutions chooseResolution(int terminalHeight, int terminalWidth) {
    const TerminalProfile high = terminalProfiles.at(Resolutions::High);
    const TerminalProfile medium = terminalProfiles.at(Resolutions::Medium);

    if (terminalWidth >= high.width && terminalHeight >= high.height) {
        return Resolutions::High;
    }

    if (terminalWidth >= medium.width && terminalHeight >= medium.height) {
        return Resolutions::Medium;
    }

    return Resolutions::Low;
}

void updateTerminalConfig(Config & cfg, int terminalHeight, int terminalWidth) {
    const TerminalProfile low = terminalProfiles.at(Resolutions::Low);

    cfg.terminalTooSmall = terminalWidth < low.width || terminalHeight < low.height;
    cfg.resolution = chooseResolution(terminalHeight, terminalWidth);
}

void selectGameScreen(ScreenManager & manager, ScreenIds id) {
    switch (id) {
        case ScreenIds::Info:         manager.curScreen = &manager.pathogen_; break;
        case ScreenIds::Cure:         manager.curScreen = &manager.cure_; break;
        case ScreenIds::World:        manager.curScreen = &manager.country_; break;
        case ScreenIds::News:         manager.curScreen = &manager.news_; break;
        case ScreenIds::Game:
        default:
            manager.curScreen = &manager.game_;
            break;
    }
}

bool applyGameRequest(Config & cfg, ScreenManager & manager, request::Game request) {
    switch (request) {
        case request::Game::Upgrade:
            cfg.id = ScreenIds::Info;
            break;
        case request::Game::Info:
            cfg.id = ScreenIds::Info;
            break;
        case request::Game::Transmission:
            cfg.id = ScreenIds::Transmission;
            break;
        case request::Game::Clinic:
            cfg.id = ScreenIds::Clinic;
            break;
        case request::Game::Abilities:
            cfg.id = ScreenIds::Abilities;
            break;
        case request::Game::World:
            cfg.id = ScreenIds::World;
            break;
        case request::Game::Cure:
            cfg.id = ScreenIds::Cure;
            break;
        case request::Game::News:
            cfg.id = ScreenIds::News;
            break;
        case request::Game::Back:
            cfg.id = ScreenIds::Game;
            break;
    }

    selectGameScreen(manager, cfg.id);
    manager.curScreen->resize();
    return true;
}

void enforceSmallTerminalScreen(const Config & cfg, ScreenManager & manager) {
    if (cfg.terminalTooSmall) {
        manager.curScreen = &manager.smallTerm_;
    }
}

}

UIManager::UIManager() {
    const char * term = std::getenv("TERM");
    if (term == nullptr || std::string_view(term) == "dumb") {
        throw std::runtime_error("Terminal does not support ncurses. Run in a real terminal or set TERM=xterm-256color.");
    }

    std::setlocale(LC_ALL, "");

    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    timeout(0);
    curs_set(0);

    if (has_colors()) {
        start_color();
        use_default_colors();
        init_pair(1, COLOR_YELLOW, -1);
        init_pair(2, COLOR_BLUE, -1);
        init_pair(3, COLOR_GREEN, -1);
    }

    int widthOfTerm, heightOfTerm;
    getmaxyx(stdscr, heightOfTerm, widthOfTerm);

    updateTerminalConfig(cfg_, heightOfTerm, widthOfTerm);

    man_ = std::make_unique<ScreenManager>(cfg_);
}

UIManager::~UIManager() {
    man_.reset();
    timeout(-1);
    keypad(stdscr, FALSE);
    curs_set(1);
    echo();
    nocbreak();
    endwin();
}

request::UIRequest UIManager::loop(GameSnapshot snap) {
    snap_ = snap;

    switch (snap_.situation) {
        case plague::GameSituation::MainMenu:        man_->curScreen = &man_->mainMenu_; break;
        case plague::GameSituation::ConnectToServer: man_->curScreen = &man_->connect_; break;
        case plague::GameSituation::ConnectingToServer: man_->curScreen = &man_->connect_; break;
        case plague::GameSituation::ChoosingSide:
            man_->choosingSide_.updateSnapshot(snap_);
            man_->curScreen = &man_->choosingSide_;
            break;
        case plague::GameSituation::Game:
            if (cfg_.id == ScreenIds::MainMenu || cfg_.id == ScreenIds::Connect || cfg_.id == ScreenIds::Settings) {
                cfg_.id = ScreenIds::Game;
            }
            selectGameScreen(*man_, cfg_.id);
            break;
        default: break;
    }

#ifdef DEBUGGAME
    man_->curScreen = &man_->news_;
#endif

    enforceSmallTerminalScreen(cfg_, *man_);
    
    int key = man_->curScreen->getKey();

    if (key == KEY_RESIZE) {
        resize();
        return request::None{};
    }

    if (key == ERR) {
        man_->curScreen->draw();
        return request::None{};
    }

    request::UIRequest request = man_->curScreen->handleInput(key);
    if (snap_.situation == plague::GameSituation::Game && std::holds_alternative<request::Game>(request)) {
        applyGameRequest(cfg_, *man_, std::get<request::Game>(request));
        request = request::None{};
    }

    man_->curScreen->draw();
    return request;
}

void UIManager::resize() {
    endwin();
    refresh();
    clearok(stdscr, TRUE);
    erase();
    refresh();

    int heightOfTerm = 0;
    int widthOfTerm = 0;
    getmaxyx(stdscr, heightOfTerm, widthOfTerm);

    updateTerminalConfig(cfg_, heightOfTerm, widthOfTerm);
    man_->resize();

    switch (snap_.situation) {
        case plague::GameSituation::MainMenu:        man_->curScreen = &man_->mainMenu_; break;
        case plague::GameSituation::ConnectToServer: man_->curScreen = &man_->connect_; break;
        case plague::GameSituation::ConnectingToServer: man_->curScreen = &man_->connect_; break;
        case plague::GameSituation::ChoosingSide:
            man_->choosingSide_.updateSnapshot(snap_);
            man_->curScreen = &man_->choosingSide_;
            break;
        case plague::GameSituation::Game:            selectGameScreen(*man_, cfg_.id); break;
        default: break;
    }

    enforceSmallTerminalScreen(cfg_, *man_);

    man_->curScreen->resize();
    man_->curScreen->draw();
}

}
