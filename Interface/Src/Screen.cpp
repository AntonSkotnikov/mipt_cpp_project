#include "Screen.hpp"
#include "Widget.hpp"

#include <cstddef>
#include <memory>

namespace plague::ui {

Screen::Screen(Config & cfg, Window & mainWin) : cfg_(cfg), win_(mainWin) {};

int Screen::getKey() {
    return win_.getKey();
}

void Screen::resize() {}

void Screen::focusFirst() {
    for (std::size_t i = 0; i < widgets.size(); i++) {
        if (widgets[i]->focusable()) {
            focusWidget(i);
            return;
        }
    }
}

void Screen::focusWidget(std::size_t index) {
    if (index >= widgets.size() || !widgets[index]->focusable()) return;

    if (focusedIndex_ < widgets.size()) {
        widgets[focusedIndex_]->setFocus(false);
    }

    focusedIndex_ = index;
    widgets[focusedIndex_]->setFocus(true);
}

void Screen::focusNext() {
    if (widgets.empty()) return;

    for (std::size_t step = 1; step <= widgets.size(); step++) {
        const std::size_t index = (focusedIndex_ + step) % widgets.size();
        if (widgets[index]->focusable()) {
            focusWidget(index);
            return;
        }
    }
}

void Screen::focusPrev() {
    if (widgets.empty()) return;

    for (std::size_t step = 1; step <= widgets.size(); step++) {
        const std::size_t index = (focusedIndex_ + widgets.size() - step) % widgets.size();
        if (widgets[index]->focusable()) {
            focusWidget(index);
            return;
        }
    }
}

void Screen::draw() {
    win_.clear();

    for (auto & widget : widgets) {
        widget->draw();
    }

    win_.refresh();
}

Widget * Screen::focusedWidget() {
    if (focusedIndex_ >= widgets.size()) return nullptr;
    return widgets[focusedIndex_].get();
}

InputResult Screen::handleFocusedInput(int key) {
    Widget * widget = focusedWidget();
    return widget == nullptr ? InputResult{} : widget->handleInput(key);
}

}  // namespace plague::ui
