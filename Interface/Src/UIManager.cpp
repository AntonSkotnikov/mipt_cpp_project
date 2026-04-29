#include "UIManager.hpp"
#include "ScreenManager.hpp"
#include "Settings.hpp"

namespace plague::ui {

UIManager::UIManager() : man_{cfg_} {
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
}

}
