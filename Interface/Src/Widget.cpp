#include "Widget.hpp"
#include "UIRequest.hpp"
#include "Upgrade.hpp"
#include "Window.hpp"
#include "utilities.hpp"
#include <algorithm>
#include <cctype>
#include <climits>
#include <cstddef>
#include <functional>
#include <iterator>
#include <memory>
#include <ncurses.h>
#include <string>
#include <vector>

namespace plague::ui {

namespace {

constexpr short selectedCountryColorPair = 1;
bool isBackspaceKey(int key) {
    return key == KEY_BACKSPACE || key == 127 || key == '\b';
}

bool isPrintableCharKey(int key) {
    return key >= 0 &&
           key <= UCHAR_MAX &&
           std::isprint(static_cast<unsigned char>(key));
}

std::string repeatText(std::string_view text, int count) {
    std::string result;
    result.reserve(text.size() * static_cast<std::size_t>(std::max(0, count)));

    for (int i = 0; i < count; i++) {
        result += text;
    }

    return result;
}

std::vector<std::string> splitLines(std::string text) {
    std::vector<std::string> lines;
    std::string current;

    for (char ch : text) {
        if (ch == '\n') {
            lines.push_back(std::move(current));
            current.clear();
        } else {
            current.push_back(ch);
        }
    }

    lines.push_back(std::move(current));
    return lines;
}

}

Widget::Widget(Window & win) : win_(win) {}

Button::Button(Window & win, std::string text, std::function<request::UIRequest()> cb) : Widget(win), text_(std::move(text)), onClick_(cb) {};

void Button::draw() {
    std::string view = text_;
    view = clipped(view, rect_.width);

    if (focused_) {
        if (has_colors()) {
            win_.attrOn(COLOR_PAIR(selectedCountryColorPair) | A_BOLD);
        } else {
            win_.attrOn(A_REVERSE);
        }
    }

    win_.print(rect_.y, rect_.x, view + repeat(' ', rect_.width - static_cast<int>(view.size())));

    if (focused_) {
        if (has_colors()) {
            win_.attrOff(COLOR_PAIR(selectedCountryColorPair) | A_BOLD);
        } else {
            win_.attrOff(A_REVERSE);
        }
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

void Button::changeText(std::string newText) {
    text_ = std::move(newText);
}

VariableInfo::VariableInfo(Window & win, std::string text) : Widget(win), line_(std::move(text)) {}

void VariableInfo::draw() {
    if (rect_.height <= 0 || rect_.width <= 0) {
        return;
    }
    win_.print(rect_.y, rect_.x, clipped(line_, rect_.width));
}

void VariableInfo::changeLine(std::string newLine) {
    line_ = std::move(newLine);
}

Ticker::Ticker(Window & win, std::size_t speed) : Widget(win), speed_(std::max<std::size_t>(1, speed)) {}

void Ticker::draw() {
    if (rect_.height <= 0 || rect_.width <= 0) {
        return;
    }

    if (curLine_.empty()) {
        loadNextLine();
    }

    const std::string visible = curLine_.substr(0, visibleLength_);
    const std::string line = clipped(visible, rect_.width);
    win_.print(rect_.y, rect_.x, line + repeat(' ', rect_.width - static_cast<int>(line.size())));

    timer_++;
    if (timer_ < speed_) {
        return;
    }

    timer_ = 0;

    if (visibleLength_ < curLine_.size()) {
        visibleLength_++;
        return;
    }

    if (!linesQueue_.empty()) {
        loadNextLine();
    }
}

void Ticker::addLine(std::string newLine) {
    linesQueue_.push_back(std::move(newLine));
}

void Ticker::loadNextLine() {
    if (linesQueue_.empty()) {
        return;
    }

    curLine_ = std::move(linesQueue_.front());
    linesQueue_.erase(linesQueue_.begin());
    visibleLength_ = curLine_.empty() ? 0 : 1;
    timer_ = 0;
}

Info::Info(Window & win, std::string text) : Widget(win) {
    changeText(std::move(text));
}

void Info::changeText(std::string newText) {
    lines_ = splitLines(std::move(newText));
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

Dialog::Dialog(Window & win, std::string text) : Widget(win), lines_(splitLines(std::move(text))) {}

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
    const std::string view = clipped(text_, rect_.width);
    win_.print(rect_.y, rect_.x, view + repeat(' ', rect_.width - static_cast<int>(view.size())));
}

InputResult TextInput::handleInput(int key) {
    if (isBackspaceKey(key)) {
        if (cursor_ > 0) {
            text_.erase(cursor_ - 1, 1);
            cursor_--;
        }
        return {true, request::None{}};
    }

    switch (key) {
        case KEY_LEFT:
            if (cursor_ > 0) {
                cursor_--;
            }
            return {true, request::None{}};

        case KEY_RIGHT:
            if (cursor_ < text_.size()) {
                cursor_++;
            }
            return {true, request::None{}};

        default:
            if (isPrintableCharKey(key)) {
                text_.insert(cursor_, 1, static_cast<char>(key));
                cursor_++;
                return {true, request::None{}};
            }
            break;
    }

    return {};
}

std::string TextInput::getText() {
    return text_;
}

DetalizedImage::DetalizedImage(Window & win) : Widget(win) {}

void DetalizedImage::draw() {
    if (rect_.height <= 0 || rect_.width <= 0) {
        return;
    }

    const bool drawSelected = focused_ && has_colors();
    if (drawSelected) {
        win_.attrOn(COLOR_PAIR(selectedCountryColorPair) | A_BOLD);
    }

    for (const SymbolOnScreen & symbol : symbols) {
        if (symbol.y < 0 || symbol.x < 0 || symbol.y >= rect_.height || symbol.x >= rect_.width) {
            continue;
        }

        win_.print(rect_.y + symbol.y, rect_.x + symbol.x, symbol.symbol);
    }

    if (drawSelected) {
        win_.attrOff(COLOR_PAIR(selectedCountryColorPair) | A_BOLD);
    }
}

void DetalizedImage::addSymbol(SymbolOnScreen newSymbol) {
    symbols.push_back(std::move(newSymbol));
}

void DetalizedImage::clearSymbols() {
    symbols.clear();
}

void DetalizedImage::addSymbols(std::vector<SymbolOnScreen> newSymbols) {
    symbols.insert(
        symbols.end(),
        std::make_move_iterator(newSymbols.begin()),
        std::make_move_iterator(newSymbols.end())
    );
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

ColorDecorator::ColorDecorator(Window & win, std::unique_ptr<Widget> inner, int colorPair)
    : WidgetDecorator(win, std::move(inner)), colorPair_(colorPair) {}

void ColorDecorator::setRect(Rect rect) {
    if (!inner_) {
        Widget::setRect(rect);
        return;
    }

    inner_->setRect(rect);
    const Rect innerRect = inner_->rect();

    Widget::setRect(innerRect);
}

void ColorDecorator::draw() {
    if (rect_.height <= 0 || rect_.width <= 0) {
        return;
    }

    win_.attrOn(COLOR_PAIR(colorPair_));

    if (inner_) {
        inner_->draw();
    }

    win_.attrOff(COLOR_PAIR(colorPair_));
}

void ColorDecorator::setColorPair(int newColorPair) {
    colorPair_ = newColorPair;
}


Menu::Menu(Window & win) : Widget(win) {}

void Menu::addButton(std::string text, std::function<request::UIRequest()> cb) {
    buttons_.push_back(std::make_unique<Button>(win_, std::move(text), std::move(cb)));
    layoutButtons();
    select(selectedIndex_);
}

void Menu::changeButtonText(std::size_t index, std::string text) {
    if (index >= buttons_.size()) {
        return;
    }

    buttons_[index]->changeText(std::move(text));
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
    const std::size_t lastVisibleIndex = std::min(buttons_.size(), firstVisibleIndex_ + count);

    for (std::size_t i = firstVisibleIndex_; i < lastVisibleIndex; i++) {
        buttons_[i]->draw();
    }
}

InputResult Menu::handleInput(int key) {
    if (buttons_.empty()) {
        return {};
    }

    switch (key) {
        case KEY_UP:
            if (selectedIndex_ == 0) return {false, request::None{}};
            select(selectedIndex_ - 1);
            return {true, request::None{}};

        case KEY_DOWN:
            if (selectedIndex_ == buttons_.size() - 1) return {false, request::None{}};
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
        firstVisibleIndex_ = 0;
        return;
    }

    const std::size_t visibleButtons = selectableCount();
    if (visibleButtons == 0) {
        for (auto & button : buttons_) {
            button->setFocus(false);
        }
        selectedIndex_ = 0;
        firstVisibleIndex_ = 0;
        return;
    }

    const int buttonWidth = std::max(1, rect_.width);
    selectedIndex_ = std::min(selectedIndex_, buttons_.size() - 1);

    if (selectedIndex_ < firstVisibleIndex_) {
        firstVisibleIndex_ = selectedIndex_;
    } else if (selectedIndex_ >= firstVisibleIndex_ + visibleButtons) {
        firstVisibleIndex_ = selectedIndex_ - visibleButtons + 1;
    }

    const std::size_t maxFirstVisibleIndex = buttons_.size() - visibleButtons;
    firstVisibleIndex_ = std::min(firstVisibleIndex_, maxFirstVisibleIndex);

    for (std::size_t i = 0; i < visibleButtons; i++) {
        const std::size_t buttonIndex = firstVisibleIndex_ + i;
        buttons_[buttonIndex]->setRect({rect_.y + static_cast<int>(i), rect_.x, 1, buttonWidth});
    }

    for (std::size_t i = 0; i < buttons_.size(); i++) {
        buttons_[i]->setFocus(focused_ && i == selectedIndex_);
    }
}

std::size_t Menu::selectableCount() const {
    if (rect_.height <= 0) {
        return 0;
    }

    return std::min(static_cast<std::size_t>(rect_.height), buttons_.size());
}

void Menu::select(std::size_t index) {
    if (buttons_.empty() || selectableCount() == 0) {
        selectedIndex_ = 0;
        firstVisibleIndex_ = 0;
        for (auto & button : buttons_) {
            button->setFocus(false);
        }
        return;
    }

    selectedIndex_ = std::min(index, buttons_.size() - 1);
    layoutButtons();
}

UpgradeList::UpgradeList(Window & win) : Widget(win) {}

void UpgradeList::setItems(std::vector<UpgradeListItem> items) {
    const std::size_t previousIndex = selectedIndex_;
    items_ = std::move(items);
    select(previousIndex);
}

void UpgradeList::setRect(Rect rect) {
    Widget::setRect(rect);
    select(selectedIndex_);
}

void UpgradeList::setFocus(bool value) {
    Widget::setFocus(value);
}

void UpgradeList::draw() {
    if (rect_.height <= 0 || rect_.width <= 0 || items_.empty()) {
        return;
    }

    const std::size_t count = selectableCount();
    const std::size_t lastVisibleIndex = std::min(items_.size(), firstVisibleIndex_ + count);

    for (std::size_t i = firstVisibleIndex_; i < lastVisibleIndex; i++) {
        const UpgradeListItem & item = items_[i];
        std::string suffix = " (" + std::to_string(item.upgrade.cost) + ")";
        if (item.purchased) {
            suffix = " [bought]";
        }

        std::string line = clipped(item.upgrade.title + suffix, rect_.width);
        line += repeat(' ', rect_.width - static_cast<int>(line.size()));

        const bool selected = focused_ && i == selectedIndex_;
        const bool available = item.available || item.purchased;

        if (selected) {
            if (has_colors()) {
                win_.attrOn(COLOR_PAIR(selectedCountryColorPair) | A_BOLD);
            } else {
                win_.attrOn(A_REVERSE);
            }
        } else if (!available) {
            win_.attrOn(A_DIM);
        }

        win_.print(rect_.y + static_cast<int>(i - firstVisibleIndex_), rect_.x, line);

        if (selected) {
            if (has_colors()) {
                win_.attrOff(COLOR_PAIR(selectedCountryColorPair) | A_BOLD);
            } else {
                win_.attrOff(A_REVERSE);
            }
        } else if (!available) {
            win_.attrOff(A_DIM);
        }
    }
}

InputResult UpgradeList::handleInput(int key) {
    if (items_.empty()) {
        return {};
    }

    switch (key) {
        case KEY_UP:
            if (selectedIndex_ == 0) return {false, request::None{}};
            select(selectedIndex_ - 1);
            return {true, request::None{}};

        case KEY_DOWN:
            if (selectedIndex_ == items_.size() - 1) return {false, request::None{}};
            select(selectedIndex_ + 1);
            return {true, request::None{}};

        case KEY_ENTER:
        case '\n':
        case '\r':
            return {true, request::None{}};
    }

    return {};
}

std::size_t UpgradeList::selectableCount() const {
    if (rect_.height <= 0) {
        return 0;
    }

    return std::min(static_cast<std::size_t>(rect_.height), items_.size());
}

void UpgradeList::select(std::size_t index) {
    if (items_.empty() || selectableCount() == 0) {
        selectedIndex_ = 0;
        firstVisibleIndex_ = 0;
        return;
    }

    selectedIndex_ = std::min(index, items_.size() - 1);

    const std::size_t visibleItems = selectableCount();
    if (selectedIndex_ < firstVisibleIndex_) {
        firstVisibleIndex_ = selectedIndex_;
    } else if (selectedIndex_ >= firstVisibleIndex_ + visibleItems) {
        firstVisibleIndex_ = selectedIndex_ - visibleItems + 1;
    }

    const std::size_t maxFirstVisibleIndex = items_.size() - visibleItems;
    firstVisibleIndex_ = std::min(firstVisibleIndex_, maxFirstVisibleIndex);
}

const UpgradeListItem * UpgradeList::selectedItem() const {
    if (selectedIndex_ >= items_.size()) {
        return nullptr;
    }

    return &items_[selectedIndex_];
}

TabBar::TabBar(Window & win) : Widget(win) {}

void TabBar::addButton(std::string text, std::function<request::UIRequest()> cb) {
    buttons_.push_back(std::make_unique<Button>(win_, std::move(text), std::move(cb)));
    layoutButtons();
    select(selectedIndex_);
}

void TabBar::setRect(Rect rect) {
    Widget::setRect(rect);
    layoutButtons();
}

void TabBar::setFocus(bool value) {
    Widget::setFocus(value);
    select(selectedIndex_);
}

void TabBar::draw() {
    const std::size_t count = selectableCount();

    for (std::size_t i = 0; i < count; i++) {
        buttons_[i]->draw();
    }
}

InputResult TabBar::handleInput(int key) {
    const std::size_t count = selectableCount();
    if (count == 0) {
        return {};
    }

    switch (key) {
        case KEY_LEFT:
        case KEY_UP:
            select(selectedIndex_ == 0 ? count - 1 : selectedIndex_ - 1);
            return {true, request::None{}};

        case KEY_RIGHT:
        case KEY_DOWN:
        case '\t':
            select((selectedIndex_ + 1) % count);
            return {true, request::None{}};

        case KEY_ENTER:
        case '\n':
        case '\r':
            return buttons_[selectedIndex_]->handleInput(key);
    }

    return {};
}

void TabBar::layoutButtons() {
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

    const int gap = count > 1 && rect_.width >= static_cast<int>(count * 8) ? 1 : 0;
    const int availableWidth = std::max(1, rect_.width - gap * static_cast<int>(count - 1));
    const int baseWidth = std::max(1, availableWidth / static_cast<int>(count));
    const int buttonY = rect_.y + std::max(0, rect_.height / 2);
    int x = rect_.x;

    for (std::size_t i = 0; i < count; i++) {
        const bool last = i == count - 1;
        const int remainingWidth = rect_.x + rect_.width - x;
        const int width = last ? std::max(1, remainingWidth) : std::min(baseWidth, std::max(1, remainingWidth));
        buttons_[i]->setRect({buttonY, x, 1, width});
        x += width + gap;
    }

    selectedIndex_ = std::min(selectedIndex_, count - 1);
    select(selectedIndex_);
}

std::size_t TabBar::selectableCount() const {
    if (rect_.height <= 0 || rect_.width <= 0) {
        return 0;
    }

    return buttons_.size();
}

void TabBar::select(std::size_t index) {
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
