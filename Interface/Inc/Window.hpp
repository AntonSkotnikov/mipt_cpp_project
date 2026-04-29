#pragma once

#include <ncurses.h>
#include <stdexcept>
#include <string>
#include <string_view>

namespace plague::ui {

inline constexpr int deltaForBorders = 2;

class NcursesError : public std::runtime_error {
public:
    explicit NcursesError(const std::string & err) : std::runtime_error(err) {};
};

class Window {
private:
    WINDOW * win_;
    int width_;
    int height_;
    int posX_;
    int posY_;
public:
    Window(int height, int width, int posY, int posX);
    Window(int height, int width); // centered

    ~Window();

    Window & operator=(const Window &) = delete;
    Window & operator=(Window &&)      = delete;
    
    Window(const Window & rhs) = delete;
    Window(Window && rhs)      = delete;

    void makeBorders();
    void refresh();
    void print(int y, int x, std::string_view text);
    void printCentered(int y, std::string_view text);
    void clear();
    int getKey();
    void attrOn(attr_t at);
    void attrOff(attr_t at);
    void colorOn(short pair);
    void colorOff(short pair);
    void resize(int height, int width);
    void move(int posY, int posX);
};

}