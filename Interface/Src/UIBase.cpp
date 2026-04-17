#include "UIBase.hpp"
#include <cstring>
#include <ncurses.h>

namespace plague::ui {

void drawCenteredText(int y, const char* text) {
    int height = 0;
    int width = 0;
    getmaxyx(stdscr, height, width);

    const int textLength = static_cast<int>(std::strlen(text));
    const int x = (width - textLength) / 2;
    mvprintw(y, x > 0 ? x : 0, "%s", text);
}



}  // namespace plague::ui
