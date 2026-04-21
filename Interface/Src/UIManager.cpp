#include "UIManager.hpp"
#include <ncurses.h>

using namespace plague::ui;

void UiManager::init() {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
}

void UiManager::shutdown() {
    endwin();
}

void UiManager::draw() {
    currentScreen().draw();
}

std::optional<plague::request::UIRequest> UiManager::pollRequest() {
    const int key = getch();

    if (key == KEY_RESIZE) {
        return std::nullopt;
    }

    const plague::request::UIRequest request = currentScreen().handleInput(key);

    return request;
}

Screen& UiManager::currentScreen() {
    switch (situation_) {
        case plague::GameSituation::MainMenu:
            return mainMenuScreen_;

        case plague::GameSituation::Settings:
            return settingsScreen_;

        default:
            return mainMenuScreen_;
    }
}