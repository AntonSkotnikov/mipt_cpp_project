#include "MainMenu.hpp"
#include "UIBase.hpp"
#include "UIRequest.hpp"

#include <ncurses.h>
#include <string_view>

using namespace plague::ui;

// TODO fix magic numbers, make logo
void MainMenuScreen::draw() const {
    clear();
    box(stdscr, 0, 0);

    drawCenteredText(2, "Plague Inc.");
    drawCenteredText(3, "Main menu");

    constexpr int startY = 6;

    int height = 0;
    int width = 0;
    getmaxyx(stdscr, height, width);

    for (int i = 0; i < 3; ++i) {
        const MainMenuItemId item = kMenuItems[i].id;
        const std::string_view text = kMenuItems[i].text;


        const int textLength = static_cast<int>(text.size());
        const int x = (width - textLength) / 2;
        const int y = startY + i * 2;

        if (item == state_.selectedItem) {
            attron(A_REVERSE);
        }

        mvprintw(y, x > 0 ? x : 0, "%s", text.data());

        if (item == state_.selectedItem) {
            attroff(A_REVERSE);
        }
    }

    mvprintw(height - 2, 2, "UP/DOWN - move | ENTER - select");
    refresh();
}

void MainMenuScreen::moveUp() {
    const MainMenuItem & item = kMenuItems[state_.selectedItem];
    if (item.up >= 0) state_.selectedItem = item.up;
}

void MainMenuScreen::moveDown() {
    const MainMenuItem & item = kMenuItems[state_.selectedItem];
    if (item.down >= 0) state_.selectedItem = item.down;
}

plague::request::UIRequest MainMenuScreen::activateCurrentItem() const {
    const MainMenuItem & item = kMenuItems[state_.selectedItem];
    return item.action;
}

plague::request::UIRequest MainMenuScreen::handleInput(int key) {
    switch (key) {
        case KEY_UP:
            moveUp();
            return plague::request::None{};

        case KEY_DOWN:
            moveDown();
            return plague::request::None{};

        case '\n':
        case '\r':
        case KEY_ENTER:
            return activateCurrentItem();

        default:
            return plague::request::None{};
    }
}

