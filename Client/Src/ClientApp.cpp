#include "ClientApp.hpp"

//минимальный пакет, просто чтобы было чем обмениваться с сервером концептуально.

namespace plague {

ClientApp::ClientApp(IUserInterface& ui, ITransport& transport)
    : ui_(ui), transport_(transport) {}

void ClientApp::run() {
    while (running_) {
        ui_.render(situation_);
        const ClientEvent event = ui_.pollEvent(situation_);
        handleEvent(event);
    }
}

void ClientApp::setSituation(GameSituation newSituation) {
    situation_ = newSituation;
}

void ClientApp::handleEvent(ClientEvent event) {
    switch (situation_) {
        case GameSituation::MainMenu:
            if (event == ClientEvent::GoToConnectMenu) {
                setSituation(GameSituation::ConnectingToServer);
            } else if (event == ClientEvent::GoToSettings) {
                setSituation(GameSituation::Settings);
            } else if (event == ClientEvent::ExitRequested) {
                setSituation(GameSituation::Exiting);
                running_ = false;
            }
            break;

        case GameSituation::Settings:
            if (event == ClientEvent::BackToMainMenu) {
                setSituation(GameSituation::MainMenu);
            }
            break;

        case GameSituation::ConnectingToServer:
            if (event == ClientEvent::SubmitConnect) {
                if (transport_.connectToServer("127.0.0.1", 5555)) {
                    transport_.send({ClientCommand::Connect, {}});
                    ui_.showMessage("Connected to server.");
                    setSituation(GameSituation::ChoosingSide);
                } else {
                    ui_.showMessage("Connection failed.");
                    setSituation(GameSituation::MainMenu);
                }
            } else if (event == ClientEvent::CancelConnect) {
                setSituation(GameSituation::MainMenu);
            }
            break;

        case GameSituation::ChoosingSide:
            if (event == ClientEvent::ChooseHumanity) {
                if (transport_.send({ClientCommand::ChooseHumanity, {}})) {
                    ui_.showMessage("Humanity selected.");
                    setSituation(GameSituation::Game);
                } else {
                    ui_.showMessage("Failed to notify server.");
                    setSituation(GameSituation::MainMenu);
                }
            } else if (event == ClientEvent::ChoosePathogen) {
                if (transport_.send({ClientCommand::ChoosePathogen, {}})) {
                    ui_.showMessage("Pathogen selected.");
                    setSituation(GameSituation::Game);
                } else {
                    ui_.showMessage("Failed to notify server.");
                    setSituation(GameSituation::MainMenu);
                }
            } else if (event == ClientEvent::DisconnectRequested) {
                transport_.send({ClientCommand::Disconnect, {}});
                transport_.disconnect();
                ui_.showMessage("Disconnected.");
                setSituation(GameSituation::MainMenu);
            }
            break;

        case GameSituation::Game:
            if (event == ClientEvent::LeaveGame) {
                setSituation(GameSituation::EndScreen);
            }
            break;

        case GameSituation::EndScreen:
            if (event == ClientEvent::BackToMainMenu) {
                setSituation(GameSituation::MainMenu);
            }
            break;

        case GameSituation::Exiting:
            running_ = false;
            break;
    }
}

}
