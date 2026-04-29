#include "InterfaceUiAdapter.hpp"

#include <ncurses.h>

namespace plague {

InterfaceUiAdapter::InterfaceUiAdapter() {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    timeout(100);
    curs_set(0);
}

InterfaceUiAdapter::~InterfaceUiAdapter() {
    endwin();
}

void InterfaceUiAdapter::render(GameSituation situation) {
    switch (situation) {
    case GameSituation::MainMenu:
        renderScreen("Main Menu", "Connect to server", "Settings", "Exit");
        break;

    case GameSituation::Settings:
        renderScreen("Settings", "Back");
        break;

    case GameSituation::ConnectToServer:
    case GameSituation::ConnectingToServer:
        renderScreen("Connecting", "Connect", "Cancel");
        break;

    case GameSituation::ConnectingToServerFailed:
        renderScreen("Connection Failed", "Back to main menu");
        break;

    case GameSituation::ChoosingSide:
        renderScreen("Choose Side", "Humanity", "Pathogen", "Disconnect");
        break;

    case GameSituation::Game:
        renderScreen("Game", "Leave game");
        break;

    case GameSituation::EndScreen:
        renderScreen("End Screen", "Back to main menu");
        break;

    case GameSituation::Exiting:
        renderScreen("Exiting...");
        break;
    }
}

request::UIRequest InterfaceUiAdapter::pollRequest(GameSituation situation) {
    const int key = getch();

    if (key == ERR || key == KEY_RESIZE) {
        return request::None{};
    }

    switch (situation) {
    case GameSituation::MainMenu:
        if (key == '1') return request::MainMenu::ConnectToServer;
        if (key == '2') return request::MainMenu::OpenSettings;
        if (key == '3') return request::MainMenu::Exit;
        break;

    case GameSituation::Settings:
        if (key == '1' || key == 27) return request::Settings::Back;
        break;

    case GameSituation::ConnectToServer:
    case GameSituation::ConnectingToServer:
        if (key == '1' || key == '\n' || key == '\r' || key == KEY_ENTER) return request::Connect::Submit;
        if (key == '2' || key == 27) return request::Connect::Cancel;
        break;

    case GameSituation::ConnectingToServerFailed:
        if (key == '1' || key == '\n' || key == '\r' || key == KEY_ENTER || key == 27) return request::Settings::Back;
        break;

    case GameSituation::ChoosingSide:
        if (key == '1') return request::SideSelection::ChooseHumanity;
        if (key == '2') return request::SideSelection::ChoosePathogen;
        if (key == '3' || key == 27) return request::SideSelection::Disconnect;
        break;

    case GameSituation::Game:
        if (key == '1' || key == '\n' || key == '\r' || key == KEY_ENTER) return request::Game::Leave;
        break;

    case GameSituation::EndScreen:
        if (key == '1' || key == '\n' || key == '\r' || key == KEY_ENTER) return request::EndScreen::BackToMainMenu;
        break;

    case GameSituation::Exiting:
        break;
    }

    return request::None{};
}

void InterfaceUiAdapter::showMessage(const char* text) {
    last_message_ = text != nullptr ? text : "";
}

void InterfaceUiAdapter::renderScreen(const char* title,
                                      const char* option1,
                                      const char* option2,
                                      const char* option3) const {
    clear();
    box(stdscr, 0, 0);

    mvprintw(2, 2, "%s", title);

    int line = 4;
    if (option1 != nullptr) {
        mvprintw(line++, 2, "1) %s", option1);
    }
    if (option2 != nullptr) {
        mvprintw(line++, 2, "2) %s", option2);
    }
    if (option3 != nullptr) {
        mvprintw(line++, 2, "3) %s", option3);
    }

    if (!last_message_.empty()) {
        mvprintw(LINES - 2, 2, "%s", last_message_.c_str());
    }

    refresh();
}

}
