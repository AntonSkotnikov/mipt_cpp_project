#include "ConsoleUi.hpp"

#include <iostream>

//ВРЕМЕННАЯ ЗАГЛУШКА интерфейса.
//Вместо нормального UI там просто консольный ввод-вывод
namespace plague {

void ConsoleUi::render(GameSituation situation) {
    std::cout << "\n====================\n";

    switch (situation) {
        case GameSituation::MainMenu:
            std::cout << "Main Menu\n";
            std::cout << "1) Connect to server\n";
            std::cout << "2) Settings\n";
            std::cout << "3) Exit\n";
            break;

        case GameSituation::Settings:
            std::cout << "Settings\n";
            std::cout << "1) Back\n";
            break;

        case GameSituation::ConnectingToServer:
            std::cout << "Connecting Screen\n";
            std::cout << "1) Connect\n";
            std::cout << "2) Cancel\n";
            break;

        case GameSituation::ChoosingSide:
            std::cout << "Choose Side\n";
            std::cout << "1) Humanity\n";
            std::cout << "2) Pathogen\n";
            std::cout << "3) Disconnect\n";
            break;

        case GameSituation::Game:
            std::cout << "Game\n";
            std::cout << "1) Leave game\n";
            break;

        case GameSituation::EndScreen:
            std::cout << "End Screen\n";
            std::cout << "1) Back to main menu\n";
            break;

        case GameSituation::Exiting:
            std::cout << "Exiting...\n";
            break;
    }

    std::cout << "====================\n";
}

ClientEvent ConsoleUi::pollEvent(GameSituation situation) {
    if (situation == GameSituation::Exiting) {
        return ClientEvent::None;
    }

    int choice = 0;
    std::cin >> choice;

    switch (situation) {
        case GameSituation::MainMenu:
            if (choice == 1) return ClientEvent::GoToConnectMenu;
            if (choice == 2) return ClientEvent::GoToSettings;
            if (choice == 3) return ClientEvent::ExitRequested;
            break;

        case GameSituation::Settings:
            if (choice == 1) return ClientEvent::BackToMainMenu;
            break;

        case GameSituation::ConnectingToServer:
            if (choice == 1) return ClientEvent::SubmitConnect;
            if (choice == 2) return ClientEvent::CancelConnect;
            break;

        case GameSituation::ChoosingSide:
            if (choice == 1) return ClientEvent::ChooseHumanity;
            if (choice == 2) return ClientEvent::ChoosePathogen;
            if (choice == 3) return ClientEvent::DisconnectRequested;
            break;

        case GameSituation::Game:
            if (choice == 1) return ClientEvent::LeaveGame;
            break;

        case GameSituation::EndScreen:
            if (choice == 1) return ClientEvent::BackToMainMenu;
            break;

        case GameSituation::Exiting:
            return ClientEvent::None;
    }

    return ClientEvent::None;
}

void ConsoleUi::showMessage(const char* text) {
    std::cout << text << '\n';
}

}
