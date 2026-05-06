#include "Window.hpp"

#include <ncurses.h>
#include <algorithm>
#include <string_view>
#include <vector>
#include <string>

namespace plague::ui {

Window::Window(int height, int width, int posY, int posX) : width_(width), height_(height), posX_(posX), posY_(posY) {
    win_ = newwin(height_, width_, posY_, posX_);
    if (win_ == nullptr) {
        throw NcursesError("Window creation error");
    }
    keypad(win_, TRUE);
    wtimeout(win_, 0);
    wrefresh(win_);
}

Window::Window(int height, int width) : width_(width), height_(height) {
    int maxY = 0;
    int maxX = 0;
    getmaxyx(stdscr, maxY, maxX);
    if (height_ > maxY) height_ = maxY;
    if (width_  > maxX) width_  = maxX;

    posY_ = (maxY - height_) / 2;
    posX_ = (maxX - width_)  / 2;

    win_ = newwin(height_, width_, posY_, posX_);
    if (win_ == nullptr) {
        throw NcursesError("Window creation error");
    }
    keypad(win_, TRUE);
    wtimeout(win_, 0);
    wrefresh(win_);
}

Window::~Window() {
    delwin(win_);
}

void Window::makeBorders() {
    ::box(win_, 0, 0);
}

void Window::setBorders(bool value) {
    bordered_ = value;
}

void Window::refresh() {
    ::wrefresh(win_);
}

void Window::print(int y, int x, std::string_view text) {
    mvwaddnstr(win_, y, x, text.data(), static_cast<int>(text.size()));
}

void Window::printCentered(int y, std::string_view text) {
    int height = 0;
    int width = 0;
    getmaxyx(win_, height, width);

    if (y >= height || width <= 0) return;

    std::vector<std::string> lines;
    std::string currentLine;

    const char * ptr = text.data();

    while (*ptr) {
        if (*ptr == '\n') {
            lines.push_back(currentLine);
            currentLine.clear();
            ++ptr;
            continue;
        }

        currentLine.push_back(*ptr);

        if ((int)currentLine.size() >= width) {
            lines.push_back(currentLine);
            currentLine.clear();
        }

        ++ptr;
    }

    if (!currentLine.empty()) {
        lines.push_back(currentLine);
    }
    
    for (size_t i = 0; i < lines.size(); ++i) {
        int currentY = y + static_cast<int>(i);
        if (currentY >= height) break;

        const std::string& line = lines[i];
        int lineLen = static_cast<int>(line.size());

        int x = (width - lineLen) / 2;
        if (x < 0) x = 0;

        mvwprintw(win_, currentY, x, "%s", line.c_str());
    }
}

void Window::clear() {
    ::werase(win_);
    if (bordered_) {
        makeBorders();
    }
}

void Window::hardClear() {
    ::wclear(win_);
    ::clearok(win_, TRUE);
    if (bordered_) {
        makeBorders();
    }
}

int Window::getKey() {
    return ::wgetch(win_);
}

void Window::attrOn(attr_t at) {
    ::wattron(win_, at);
}

void Window::attrOff(attr_t at) {
    ::wattroff(win_, at);
}

void Window::colorOn(short pair) {
    ::wattron(win_, COLOR_PAIR(pair));
}

void Window::colorOff(short pair) {
    ::wattroff(win_, COLOR_PAIR(pair));
}

void Window::resize(int height, int width) {
    height = std::max(1, height);
    width = std::max(1, width);

    if (wresize(win_, height, width) == ERR) {
        throw NcursesError("Resize error");
    }
    width_ = width;
    height_ = height;
    hardClear();
}

void Window::resizeCentered(int height, int width) {
    int maxY = 0;
    int maxX = 0;
    getmaxyx(stdscr, maxY, maxX);

    if (maxY <= 0 || maxX <= 0) {
        throw NcursesError("Terminal size error");
    }

    height = std::clamp(height, 1, maxY);
    width = std::clamp(width, 1, maxX);

    resize(height, width);
    move((maxY - height) / 2, (maxX - width) / 2);
}

void Window::move(int posY, int posX) {
    if (mvwin(win_, posY, posX) == ERR) {
        throw NcursesError("Move error");
    }
    posY_ = posY;
    posX_ = posX;
    hardClear();
}

}
