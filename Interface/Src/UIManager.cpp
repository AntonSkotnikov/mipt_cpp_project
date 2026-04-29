#include "UIManager.hpp"
#include "ScreenManager.hpp"
#include "Settings.hpp"
#include "UIRequest.hpp"
#include "UI_ClientAPI.hpp"
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

}

UIManager::UIManager() {
    const char * term = std::getenv("TERM");
    if (term == nullptr || std::string_view(term) == "dumb") {
        throw std::runtime_error("Terminal does not support ncurses. Run in a real terminal or set TERM=xterm-256color.");
    }

    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);

    int widthOfTerm, heightOfTerm;
    getmaxyx(stdscr, heightOfTerm, widthOfTerm);

    cfg_.resolution = chooseResolution(heightOfTerm, widthOfTerm);

    man_ = std::make_unique<ScreenManager>(cfg_);
}

UIManager::~UIManager() {
    endwin();
}

request::UIRequest UIManager::loop(GameSnapshot snap) {
    snap_ = snap;
    man_->curScreen->draw();
    int key = man_->curScreen->getKey();

    if (key == KEY_RESIZE) {
        resize();
        return request::None{};
    }

    return man_->curScreen->handleInput(key);
}

void UIManager::resize() {
    refresh();
    clear();
    refresh();

    int heightOfTerm = 0;
    int widthOfTerm = 0;
    getmaxyx(stdscr, heightOfTerm, widthOfTerm);

    cfg_.resolution = chooseResolution(heightOfTerm, widthOfTerm);
    man_->resize();
    man_->curScreen->draw();
}

}
