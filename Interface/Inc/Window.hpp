#pragma once

#include <ncurses.h>
#include <stdexcept>
#include <string>
#include <string_view>

namespace plague::ui {

/** @brief Extra rows/columns used when the main window draws a border. */
inline constexpr int deltaForBorders = 2;

/**
 * @brief Exception raised by the ncurses window wrapper.
 */
class NcursesError : public std::runtime_error {
public:
    explicit NcursesError(const std::string & err) : std::runtime_error(err) {};
};

/**
 * @brief RAII wrapper around an ncurses WINDOW.
 *
 * Window centralizes printing, resizing, borders, attributes, input polling,
 * and safe cleanup for all UI screens and widgets.
 */
class Window {
private:
    WINDOW * win_;
    int width_;
    int height_;
    int posX_;
    int posY_;
    bool bordered_ = false;
public:
    /**
     * @brief Create a window with explicit size and position.
     * @param height Window height in terminal rows.
     * @param width Window width in terminal columns.
     * @param posY Top-left row.
     * @param posX Top-left column.
     */
    Window(int height, int width, int posY, int posX);
    /**
     * @brief Create a centered window with the requested size.
     * @param height Requested height in terminal rows.
     * @param width Requested width in terminal columns.
     */
    Window(int height, int width); // centered

    /** @brief Destroy the underlying ncurses WINDOW. */
    ~Window();

    Window & operator=(const Window &) = delete;
    Window & operator=(Window &&)      = delete;
    
    Window(const Window & rhs) = delete;
    Window(Window && rhs)      = delete;

    /** @brief Draw a box border around the whole window. */
    void makeBorders();
    /** @brief Enable or disable border drawing during clears. */
    void setBorders(bool value);
    /** @return true if borders are enabled for this window. */
    bool bordered() const { return bordered_; }
    /** @brief Flush pending ncurses drawing to the terminal. */
    void refresh();
    /**
     * @brief Print text at a window-relative position.
     * @param y Row inside the window.
     * @param x Column inside the window.
     * @param text Text to print. ASCII text is clipped to the window width.
     */
    void print(int y, int x, std::string_view text);
    /** @brief Print possibly multi-line text centered horizontally. */
    void printCentered(int y, std::string_view text);
    /** @brief Clear the window and redraw borders when enabled. */
    void clear();
    /** @brief Force-clear the window and redraw borders when enabled. */
    void hardClear();
    /** @return The next input key, or ERR when no input is available. */
    int getKey();
    /** @brief Enable ncurses attributes. */
    void attrOn(attr_t at);
    /** @brief Disable ncurses attributes. */
    void attrOff(attr_t at);
    /** @brief Enable an ncurses color pair. */
    void colorOn(short pair);
    /** @brief Disable an ncurses color pair. */
    void colorOff(short pair);
    /** @brief Resize the underlying window. */
    void resize(int height, int width);
    /** @brief Resize and recenter the window in the current terminal. */
    void resizeCentered(int height, int width);
    /** @brief Move the window to a new terminal position. */
    void move(int posY, int posX);
    /** @return Current window height in rows. */
    int height() const { return height_; }
    /** @return Current window width in columns. */
    int width() const { return width_; }
};

}
