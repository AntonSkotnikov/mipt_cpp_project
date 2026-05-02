#include "Widget.hpp"
#include "UIRequest.hpp"
#include "Window.hpp"
#include "utilities.hpp"
#include <algorithm>
#include <cctype>
#include <cstddef>
#include <functional>
#include <memory>
#include <ncurses.h>
#include <string>

namespace plague::ui {

namespace {

std::string repeatText(std::string_view text, int count) {
    std::string result;

    for (int i = 0; i < count; i++) {
        result += text;
    }

    return result;
}

}

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

Dialog::Dialog(Window & win, std::string text) : Widget(win) {
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

void Dialog::addButton(std::string text, std::function<request::UIRequest()> cb) {
    buttons_.push_back(std::make_unique<Button>(win_, std::move(text), std::move(cb)));
    layoutButtons();
    select(selectedIndex_);
}

void Dialog::setRect(Rect rect) {
    Widget::setRect(rect);
    layoutButtons();
}

void Dialog::setFocus(bool value) {
    Widget::setFocus(value);
    select(selectedIndex_);
}

void Dialog::draw() {
    if (rect_.height <= 0 || rect_.width <= 0) {
        return;
    }

    const int buttonCount = static_cast<int>(selectableCount());
    const int textHeight = std::max(1, rect_.height - buttonCount - (buttonCount > 0 ? 1 : 0));
    const int contentY = rect_.y + std::max(0, (textHeight - static_cast<int>(lines_.size())) / 2);

    for (std::size_t i = 0; i < lines_.size(); i++) {
        const std::string line = clipped(lines_[i], rect_.width);
        const int x = rect_.x + std::max(0, (rect_.width - static_cast<int>(line.size())) / 2);
        const int y = contentY + static_cast<int>(i);

        if (y >= rect_.y && y < rect_.y + textHeight) {
            win_.print(y, x, line);
        }
    }

    const std::size_t count = selectableCount();
    for (std::size_t i = 0; i < count; i++) {
        buttons_[i]->draw();
    }
}

InputResult Dialog::handleInput(int key) {
    const std::size_t count = selectableCount();
    if (count == 0) {
        return {};
    }

    switch (key) {
        case KEY_UP:
        case KEY_LEFT:
            select(selectedIndex_ == 0 ? count - 1 : selectedIndex_ - 1);
            return {true, request::None{}};

        case KEY_DOWN:
        case KEY_RIGHT:
        case '\t':
            select((selectedIndex_ + 1) % count);
            return {true, request::None{}};

        case KEY_ENTER:
        case '\n':
            return buttons_[selectedIndex_]->handleInput(key);
    }

    return {};
}

void Dialog::layoutButtons() {
    if (buttons_.empty()) {
        selectedIndex_ = 0;
        return;
    }

    const std::size_t count = selectableCount();
    if (count == 0) {
        for (auto & button : buttons_) {
            button->setFocus(false);
        }
        selectedIndex_ = 0;
        return;
    }

    const int buttonWidth = std::min(24, std::max(1, rect_.width));
    const int buttonX = rect_.x + std::max(0, (rect_.width - buttonWidth) / 2);
    const int firstButtonY = rect_.y + rect_.height - static_cast<int>(count);

    for (std::size_t i = 0; i < count; i++) {
        buttons_[i]->setRect({firstButtonY + static_cast<int>(i), buttonX, 1, buttonWidth});
    }

    selectedIndex_ = std::min(selectedIndex_, count - 1);
    select(selectedIndex_);
}

std::size_t Dialog::selectableCount() const {
    if (rect_.height <= 0) {
        return 0;
    }

    return std::min(static_cast<std::size_t>(rect_.height), buttons_.size());
}

void Dialog::select(std::size_t index) {
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

TextInput::TextInput(Window & win) : Widget(win) {}

void TextInput::draw() {
    win_.print(rect_.y, rect_.x, text_);
}

InputResult TextInput::handleInput(int key) {
    switch (key) {
        case KEY_BACKSPACE:
            if (cursor_ > 0) {
                text_.erase(cursor_ - 1, 1);
                cursor_--;
            }
            break;
        case KEY_LEFT:
            if (cursor_ > 0) {
                cursor_--;
            }
            break;

        case KEY_RIGHT:
            if (cursor_ < text_.size()) {
                cursor_++;
            }
            break;

        default:
            if (std::isprint(key)) {
                text_.insert(cursor_, 1, static_cast<char>(key));
                cursor_++;
            }
            break;
    }

    return {};
}

std::string TextInput::getText() {
    return text_;
}


// Decorators
WidgetDecorator::WidgetDecorator(Window & win, std::unique_ptr<Widget> inner) : Widget(win), inner_(std::move(inner)) {}

void WidgetDecorator::setFocus(bool value) {
    Widget::setFocus(value);
    if (inner_) {
        inner_->setFocus(value);
    }
}

InputResult WidgetDecorator::handleInput(int key) {
    if (!inner_) {
        return {};
    }

    return inner_->handleInput(key);
}

bool WidgetDecorator::focusable() const {
    return inner_ != nullptr && inner_->focusable();
}

FrameDecorator::FrameDecorator(Window & win, std::unique_ptr<Widget> inner) : WidgetDecorator(win, std::move(inner)) {}

void FrameDecorator::setRect(Rect rect) {
    if (!inner_) {
        Widget::setRect(rect);
        return;
    }

    inner_->setRect(rect);
    const Rect innerRect = inner_->rect();
    Widget::setRect({innerRect.y - 1, innerRect.x - 1, innerRect.height + 2, innerRect.width + 2});
}

void FrameDecorator::draw() {
    if (rect_.height <= 0 || rect_.width <= 0) {
        return;
    }

    if (rect_.height == 1) {
        win_.print(rect_.y, rect_.x, repeatText("─", rect_.width));
    } else if (rect_.width == 1) {
        for (int y = 0; y < rect_.height; y++) {
            win_.print(rect_.y + y, rect_.x, "│");
        }
    } else {
        win_.print(rect_.y, rect_.x, "╭" + repeatText("─", rect_.width - 2) + "╮");

        for (int y = 1; y < rect_.height - 1; y++) {
            win_.print(rect_.y + y, rect_.x, "│" + repeat(' ', rect_.width - 2) + "│");
        }

        win_.print(rect_.y + rect_.height - 1, rect_.x, "╰" + repeatText("─", rect_.width - 2) + "╯");
    }

    if (inner_) {
        inner_->draw();
    }
}

LabelDecorator::LabelDecorator(Window & win, std::unique_ptr<Widget> inner, std::string label)
    : WidgetDecorator(win, std::move(inner)), label_(std::move(label)) {}

void LabelDecorator::setRect(Rect rect) {
    if (!inner_) {
        Widget::setRect(rect);
        return;
    }

    inner_->setRect(rect);
    const Rect innerRect = inner_->rect();

    if (label_.empty()) {
        Widget::setRect(innerRect);
        return;
    }

    Widget::setRect({innerRect.y - 1, innerRect.x, innerRect.height + 1, innerRect.width});
}

void LabelDecorator::draw() {
    if (rect_.height <= 0 || rect_.width <= 0) {
        return;
    }

    if (!label_.empty()) {
        const std::string label = clipped(label_, rect_.width);
        win_.print(rect_.y, rect_.x, label + repeat(' ', rect_.width - static_cast<int>(label.size())));
    }

    if (inner_) {
        inner_->draw();
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
            if (selectedIndex_ == 0) return {false, request::None{}};
            select(selectedIndex_ - 1);
            return {true, request::None{}};

        case KEY_DOWN:
            if (selectedIndex_ == count - 1) return {false, request::None{}};
            select(selectedIndex_ + 1);
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
