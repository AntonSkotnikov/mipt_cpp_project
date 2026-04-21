#include "InterfaceUiAdapter.hpp"

#include <ncurses.h>

namespace plague {

namespace {

void drawBoxedScreen(const char* title,
                     const char* option1 = nullptr,
                     const char* option2 = nullptr,
                     const char* option3 = nullptr,
                     const char* message = nullptr) {
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

    if (message != nullptr && message[0] != '\0') {
        mvprintw(LINES - 2, 2, "%s", message);
    }

    refresh();
}

}  // namespace

InterfaceUiAdapter::InterfaceUiAdapter() {
    manager_.init();
}

InterfaceUiAdapter::~InterfaceUiAdapter() {
    manager_.shutdown();
}

void InterfaceUiAdapter::render(GameSituation situation) {
    if (isManagedByUiManager(situation)) {
        manager_.setSituation(situation);
        manager_.draw();
        if (!last_message_.empty()) {
            mvprintw(LINES - 2, 2, "%s", last_message_.c_str());
            refresh();
        }
        return;
    }

    renderFallback(situation);
}

request::UIRequest InterfaceUiAdapter::pollRequest(GameSituation situation) {
    if (isManagedByUiManager(situation)) {
        manager_.setSituation(situation);
        const auto request = manager_.pollRequest();
        return request.value_or(request::None{});
    }

    return pollFallbackRequest(situation);
}

void InterfaceUiAdapter::showMessage(const char* text) {
    last_message_ = text != nullptr ? text : "";
}

bool InterfaceUiAdapter::isManagedByUiManager(GameSituation situation) const {
    return situation == GameSituation::MainMenu || situation == GameSituation::Settings;
}

void InterfaceUiAdapter::renderFallback(GameSituation situation) const {
    switch (situation) {
    case GameSituation::ConnectingToServer:
        drawBoxedScreen("Connecting", "Connect", "Cancel", nullptr, last_message_.c_str());
        break;

    case GameSituation::ChoosingSide:
        drawBoxedScreen("Choose Side", "Humanity", "Pathogen", "Disconnect", last_message_.c_str());
        break;

    case GameSituation::Game:
        drawBoxedScreen("Game", "Leave game", nullptr, nullptr, last_message_.c_str());
        break;

    case GameSituation::EndScreen:
        drawBoxedScreen("End Screen", "Back to main menu", nullptr, nullptr, last_message_.c_str());
        break;

    case GameSituation::Exiting:
        drawBoxedScreen("Exiting...", nullptr, nullptr, nullptr, last_message_.c_str());
        break;

    case GameSituation::MainMenu:
    case GameSituation::Settings:
        break;
    }
}

request::UIRequest InterfaceUiAdapter::pollFallbackRequest(GameSituation situation) const {
    const int key = getch();

    if (key == KEY_RESIZE) {
        return request::None{};
    }

    switch (situation) {
    case GameSituation::ConnectingToServer:
        if (key == '1' || key == '\n' || key == '\r' || key == KEY_ENTER) {
            return request::Connect::Submit;
        }
        if (key == '2' || key == 27) {
            return request::Connect::Cancel;
        }
        break;

    case GameSituation::ChoosingSide:
        if (key == '1') {
            return request::SideSelection::ChooseHumanity;
        }
        if (key == '2') {
            return request::SideSelection::ChoosePathogen;
        }
        if (key == '3' || key == 27) {
            return request::SideSelection::Disconnect;
        }
        break;

    case GameSituation::Game:
        if (key == '1' || key == '\n' || key == '\r' || key == KEY_ENTER) {
            return request::Game::Leave;
        }
        break;

    case GameSituation::EndScreen:
        if (key == '1' || key == '\n' || key == '\r' || key == KEY_ENTER) {
            return request::EndScreen::BackToMainMenu;
        }
        break;

    case GameSituation::Exiting:
    case GameSituation::MainMenu:
    case GameSituation::Settings:
        break;
    }

    return request::None{};
}

}
