#include "Widget.hpp"
#include "UIRequest.hpp"
#include "Window.hpp"
#include "utilities.hpp"
#include <functional>
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

request::UIRequest Button::handleInput(int key) {
    if (key == '\n' || key == KEY_ENTER || key == '\r') {
        if (onClick_) {
            return onClick_();
        }
    }

    return request::None{};
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

}