#include "UIManager.hpp"
#include "ScreenManager.hpp"
#include "Settings.hpp"
#include "UIRequest.hpp"
#include <cstdlib>
#include <stdexcept>
#include <ncurses.h>

namespace plague::ui {

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

    if (widthOfTerm < terminalProfiles.at(Resolutions::Low).width || heightOfTerm < terminalProfiles.at(Resolutions::Low).height) {
        // make alert
    } else if (widthOfTerm < terminalProfiles.at(Resolutions::Medium).width || heightOfTerm < terminalProfiles.at(Resolutions::Medium).height) {
        cfg_.resolution = Resolutions::Low;
    }

    man_ = std::make_unique<ScreenManager>(cfg_);
}

UIManager::~UIManager() {
    endwin();
}

request::UIRequest UIManager::loop() {
    man_->curScreen->draw();
    int key = man_->curScreen->getKey();
    return man_->curScreen->handleInput(key);
}

}
