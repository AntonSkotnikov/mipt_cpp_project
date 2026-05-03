#include "InterfaceUiAdapter.hpp"

#include <ncurses.h>

namespace plague {

namespace {
int g_current_selection = 1;
}

InterfaceUiAdapter::InterfaceUiAdapter() {
    manager_ = std::make_unique<ui::UIManager>();
    timeout(100);
}

void InterfaceUiAdapter::render(const GameSnapshot& snapshot) {
    if (usesInterfaceLoop(snapshot)) {
        return;
    }

    switch (snapshot.situation) {
    case GameSituation::MainMenu:
        renderScreen(snapshot, "Main Menu", "Connect to server", "Settings", "Exit");
        break;

    case GameSituation::Settings:
        renderScreen(snapshot, "Settings", "Back");
        break;

    case GameSituation::ConnectToServer:
    case GameSituation::ConnectingToServer:
        renderScreen(snapshot, "Connecting", "Connect", "Cancel");
        break;

    case GameSituation::ConnectingToServerFailed:
        renderScreen(snapshot, "Connection Failed", "Back to main menu");
        break;

    case GameSituation::ChoosingSide:
        renderScreen(snapshot, "Choose Side", "Humanity", "Pathogen", "Disconnect");
        break;

    case GameSituation::Game:
        renderScreen(snapshot, "Game", "Leave game");
        break;

    case GameSituation::EndScreen:
        renderScreen(snapshot, "End Screen", "Back to main menu");
        break;

    case GameSituation::Exiting:
        renderScreen(snapshot, "Exiting...");
        break;
    }
}

request::UIRequest InterfaceUiAdapter::pollRequest(const GameSnapshot& snapshot) {
    if (usesInterfaceLoop(snapshot)) {
        return manager_->loop(snapshot);
    }

    const int key = getch();

    if (key == ERR || key == KEY_RESIZE) {
        return request::None{};
    }

    int num_options = 0;
    switch (snapshot.situation) {
    case GameSituation::Settings:
    case GameSituation::ConnectingToServerFailed:
    case GameSituation::Game:
    case GameSituation::EndScreen:
        num_options = 1; break;
    case GameSituation::ConnectToServer:
    case GameSituation::ConnectingToServer:
        num_options = 2; break;
    case GameSituation::ChoosingSide:
        num_options = 3; break;
    default: break;
    }

    if (key == KEY_UP && num_options > 0) {
        g_current_selection = (g_current_selection > 1) ? g_current_selection - 1 : num_options;
    } else if (key == KEY_DOWN && num_options > 0) {
        g_current_selection = (g_current_selection < num_options) ? g_current_selection + 1 : 1;
    }

    switch (snapshot.situation) {
    case GameSituation::MainMenu:
        if (key == '1') return request::MainMenu::ConnectToServer;
        if (key == '2') return request::MainMenu::OpenSettings;
        if (key == '3') return request::MainMenu::Exit;
        break;

    case GameSituation::Settings:
        if (key == '\n' || key == '\r' || key == KEY_ENTER || key == '1' || key == 27) return request::Settings::Back;
        break;

    case GameSituation::ConnectToServer:
    case GameSituation::ConnectingToServer:
        if (key == '\n' || key == '\r' || key == KEY_ENTER) {
            if (g_current_selection == 1) return request::ConnectInfo{request::Connect::Connect, "", ""};
            if (g_current_selection == 2) return request::ConnectInfo{request::Connect::Back, "", ""};
        }
        if (key == '1') return request::ConnectInfo{request::Connect::Connect, "", ""};
        if (key == '2' || key == 27) return request::ConnectInfo{request::Connect::Back, "", ""};
        break;

    case GameSituation::ConnectingToServerFailed:
        if (key == '\n' || key == '\r' || key == KEY_ENTER || key == '1' || key == 27) return request::Settings::Back;
        break;

    case GameSituation::ChoosingSide:
        if (key == '\n' || key == '\r' || key == KEY_ENTER) {
            if (g_current_selection == 1) return request::MainMenu::ConnectToServer;
            if (g_current_selection == 2) return request::MainMenu::OpenSettings;
            if (g_current_selection == 3) return request::MainMenu::Exit;
        }
        if (key == '1') return request::MainMenu::ConnectToServer;
        if (key == '2') return request::MainMenu::OpenSettings;
        if (key == '3' || key == 27) return request::MainMenu::Exit;
        break;

    case GameSituation::Game:
        if (key == '\n' || key == '\r' || key == KEY_ENTER || key == '1') return request::Settings::Back;
        break;

    case GameSituation::EndScreen:
        if (key == '\n' || key == '\r' || key == KEY_ENTER || key == '1') return request::Settings::Back;
        break;

    case GameSituation::Exiting:
        break;
    }

    return request::None{};
}

void InterfaceUiAdapter::showMessage(const char* text) {
    last_message_ = text != nullptr ? text : "";
}

bool InterfaceUiAdapter::usesInterfaceLoop(const GameSnapshot& snapshot) const {
    return snapshot.situation == GameSituation::MainMenu;
}

void InterfaceUiAdapter::renderScreen(const GameSnapshot& snapshot,
                                      const char* title,
                                      const char* option1,
                                      const char* option2,
                                      const char* option3) const {
    clear();
    box(stdscr, 0, 0);

    mvprintw(2, 2, "%s", title);

    int num_options = 0;
    if (option1 != nullptr) num_options++;
    if (option2 != nullptr) num_options++;
    if (option3 != nullptr) num_options++;

    if (g_current_selection < 1) g_current_selection = 1;
    if (num_options > 0 && g_current_selection > num_options) {
        g_current_selection = num_options;
    }

    int line = 4;
    if (option1 != nullptr) {
        if (g_current_selection == 1) attron(A_REVERSE);
        mvprintw(line++, 2, "1) %s", option1);
        if (g_current_selection == 1) attroff(A_REVERSE);
    }
    if (option2 != nullptr) {
        if (g_current_selection == 2) attron(A_REVERSE);
        mvprintw(line++, 2, "2) %s", option2);
        if (g_current_selection == 2) attroff(A_REVERSE);
    }
    if (option3 != nullptr) {
        if (g_current_selection == 3) attron(A_REVERSE);
        mvprintw(line++, 2, "3) %s", option3);
        if (g_current_selection == 3) attroff(A_REVERSE);
    }

    mvprintw(line + 1, 2, "Day: %u", static_cast<unsigned>(snapshot.day));
    mvprintw(line + 2, 2, "Role: %s", snapshot.playerInfo.role == PlayerRole::Humanity ? "Humanity" : "Pathogen");
    mvprintw(line + 3, 2, "Points: %u", static_cast<unsigned>(snapshot.playerInfo.points));

    if (!snapshot.recentNews.empty()) {
        mvprintw(line + 5, 2, "News: %s", snapshot.recentNews.back().text_.c_str());
    }

    if (!last_message_.empty()) {
        mvprintw(LINES - 2, 2, "%s", last_message_.c_str());
    }

    refresh();
}

}
