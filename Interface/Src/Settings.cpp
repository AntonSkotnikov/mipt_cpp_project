#include "UIBase.hpp"
#include "Settings.hpp"
#include <ncurses.h>
#include <string_view>

using namespace plague::ui;

void SettingsScreen::moveUp() {
    const SettingsItem & item = kSettingsItems[state_.selectedItem];
    if (item.up >= 0) state_.selectedItem = item.up;
}

void SettingsScreen::moveDown() {
    const SettingsItem & item = kSettingsItems[state_.selectedItem];
    if (item.down >= 0) state_.selectedItem = item.down;
}

void SettingsScreen::moveRight() {
    if (!isChanging_) return;
    auto & val = state_.settings[state_.selectedItem];
    if (isChanging_ && val < kSettingsItems[state_.selectedItem].maxValue) val++;
}

void SettingsScreen::moveLeft() {
    if (!isChanging_) return;
    auto & val = state_.settings[state_.selectedItem];
    if (isChanging_ && val > kSettingsItems[state_.selectedItem].minValue) val--;
}

void SettingsScreen::endChangeWithSave() {
    if (!isChanging_) return;
    isChanging_ = 0;
}

void SettingsScreen::endChangeWithoutSave() {
    if (!isChanging_) return;
    state_.settings[state_.selectedItem] = dump_;
    isChanging_ = 0;
}

plague::request::UIRequest SettingsScreen::activateCurrentItem() {
    if (isChanging_) {
        endChangeWithSave();
        return request::None{};
    }
    if (state_.selectedItem != Back) {
        dump_ = state_.settings[state_.selectedItem];
        isChanging_ = true;
    }

    const SettingsItem & item = kSettingsItems[state_.selectedItem];
    return item.action;
}

// TODO scroll
void SettingsScreen::draw() const {
    clear();
    box(stdscr, 0, 0);

    int height = 0;
    int width = 0;
    getmaxyx(stdscr, height, width);

    drawCenteredText(1, "Settings");

    for (size_t i = 0; i < SettingsItemId::Back; i++) {
        if (i == state_.selectedItem && !isChanging_) attron(A_REVERSE);
        mvprintw(i + 2, 2, "%s", kSettingsItems[i].textOfSetting.data()); // TODO fix magic numbers
        if (i == state_.selectedItem && !isChanging_) attroff(A_REVERSE);

        if (i == state_.selectedItem && isChanging_) attron(A_REVERSE);
        std::string_view valText = kSettingsItems[i].possibleValues[state_.settings[i]];
        mvprintw(i + 2, width - 4 - valText.size(), "<%s>", valText.data()); // TODO: make not only with init values; fix x; fix magic numbers
        if (i == state_.selectedItem && isChanging_) attroff(A_REVERSE);
    }

    if (state_.selectedItem == SettingsItemId::Back) attron(A_REVERSE);
    mvprintw(height - 4, 2, "%s", kSettingsItems[SettingsItemId::Back].textOfSetting.data());
    if (state_.selectedItem == SettingsItemId::Back) attroff(A_REVERSE);

    mvprintw(LINES - 2, 2, "UP/DOWN - move | LEFT/RIGHT - change | ENTER - select | ESC - end change without save");
    refresh();
}

plague::request::UIRequest SettingsScreen::handleInput(int key) {
    switch (key) {
        case KEY_UP:    moveUp();   break;
        case KEY_DOWN:  moveDown(); break;
        case KEY_LEFT:  moveLeft(); break;
        case KEY_RIGHT: moveRight(); break;

        case '\n': case KEY_ENTER: case 13: return activateCurrentItem();
        case 27: endChangeWithoutSave(); break;
        default:
            return plague::request::None{};
    }

    return request::None{};
}