#include "Widget.hpp"
#include "UIRequest.hpp"
#include "Window.hpp"
#include "utilities.hpp"
#include <algorithm>
#include <cstddef>
#include <functional>
#include <memory>
#include <ncurses.h>
#include <string>

namespace plague::ui {

Widget::Widget(Window & win) : win_(win) {}

Button::Button(Window & win, std::string text, std::function<request::UIRequest()> cb) : Widget(win), text_(std::move(text)), onClick_(cb) {};

void Button::draw() {
    std::string view = text_;
    view = clipped(view, rect_.width);

    if (focused_) {
        win_.attrOn(A_REVERSE);
    }

    win_.print(rect_.y, rect_.x, view + repeat(' ', rect_.width - static_cast<int>(view.size())));

    if (focused_) {
        win_.attrOff(A_REVERSE);
    }
}

InputResult Button::handleInput(int key) {
    if (key == '\n' || key == KEY_ENTER || key == '\r') {
        if (onClick_) {
            return {true, onClick_()};
        }

        return {true, request::None{}};
    }

    return {};
}

Info::Info(Window & win, std::string text) : Widget(win) {
    lines_.clear();

    std::string current;

    for (char ch : text) {
        if (ch == '\n') {
            lines_.push_back(std::move(current));
            current.clear();
        } else {
            current.push_back(ch);
        }
    }

    lines_.push_back(std::move(current));
}

void Info::draw() {
    if (rect_.height <= 0 || rect_.width <= 0) {
        return;
    }

    int count = std::min(rect_.height, static_cast<int>(lines_.size()));

    for (int i = 0; i < count; i++) {
        win_.print(rect_.y + i, rect_.x, clipped(lines_[static_cast<std::size_t>(i)], rect_.width));
    }
}

Menu::Menu(Window & win) : Widget(win) {}

void Menu::addButton(std::string text, std::function<request::UIRequest()> cb) {
    buttons_.push_back(std::make_unique<Button>(win_, std::move(text), std::move(cb)));
    layoutButtons();
    select(selectedIndex_);
}

void Menu::setRect(Rect rect) {
    Widget::setRect(rect);
    layoutButtons();
}

void Menu::setFocus(bool value) {
    Widget::setFocus(value);
    select(selectedIndex_);
}

void Menu::draw() {
    const std::size_t count = selectableCount();

    for (std::size_t i = 0; i < count; i++) {
        buttons_[i]->draw();
    }
}

InputResult Menu::handleInput(int key) {
    const std::size_t count = selectableCount();
    if (count == 0) {
        return {};
    }

    switch (key) {
        case KEY_UP:
            select(selectedIndex_ == 0 ? count - 1 : selectedIndex_ - 1);
            return {true, request::None{}};

        case KEY_DOWN:
            select((selectedIndex_ + 1) % count);
            return {true, request::None{}};

        case KEY_ENTER: case '\n':
            return buttons_[selectedIndex_]->handleInput(key);
    }

    return {};
}

void Menu::layoutButtons() {
    if (buttons_.empty()) {
        selectedIndex_ = 0;
        return;
    }

    const std::size_t visibleButtons = selectableCount();
    if (visibleButtons == 0) {
        for (auto & button : buttons_) {
            button->setFocus(false);
        }
        selectedIndex_ = 0;
        return;
    }

    const int buttonWidth = std::max(1, rect_.width);

    for (std::size_t i = 0; i < visibleButtons; i++) {
        buttons_[i]->setRect({rect_.y + static_cast<int>(i), rect_.x, 1, buttonWidth});
    }

    selectedIndex_ = std::min(selectedIndex_, visibleButtons - 1);
    select(selectedIndex_);
}

std::size_t Menu::selectableCount() const {
    if (rect_.height <= 0) {
        return 0;
    }

    return std::min(static_cast<std::size_t>(rect_.height), buttons_.size());
}

void Menu::select(std::size_t index) {
    const std::size_t count = selectableCount();
    if (count == 0) {
        selectedIndex_ = 0;
        for (auto & button : buttons_) {
            button->setFocus(false);
        }
        return;
    }

    selectedIndex_ = std::min(index, count - 1);

    for (std::size_t i = 0; i < buttons_.size(); i++) {
        buttons_[i]->setFocus(focused_ && i == selectedIndex_);
    }
}

}
