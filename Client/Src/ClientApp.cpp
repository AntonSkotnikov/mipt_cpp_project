#include "ClientApp.hpp"

namespace plague {

ClientApp::ClientApp(IUserInterface& ui, ITransport& transport)
    : ui_(ui), transport_(transport), request_handler_(std::make_unique<RequestHandler>(transport)) {}

void ClientApp::run() {
    while (running_) {
        ui_.render(situation_);
        const request::UIRequest request = ui_.pollRequest(situation_);
        handleRequest(request);
        request_handler_->update();
    }
}

void ClientApp::setSituation(GameSituation newSituation) {
    situation_ = newSituation;
}

void ClientApp::handleRequest(const request::UIRequest& request) {
    switch (situation_) {
    case GameSituation::MainMenu:
        if (std::holds_alternative<request::MainMenu>(request) &&
            std::get<request::MainMenu>(request) == request::MainMenu::ConnectToServer) {
            setSituation(GameSituation::ConnectingToServer);
        } else if (std::holds_alternative<request::MainMenu>(request) &&
                   std::get<request::MainMenu>(request) == request::MainMenu::OpenSettings) {
            setSituation(GameSituation::Settings);
        } else if (std::holds_alternative<request::MainMenu>(request) &&
                   std::get<request::MainMenu>(request) == request::MainMenu::Exit) {
            setSituation(GameSituation::Exiting);
            running_ = false;
        }
        break;

    case GameSituation::Settings:
        if (std::holds_alternative<request::Settings>(request) &&
            std::get<request::Settings>(request) == request::Settings::Back) {
            setSituation(GameSituation::MainMenu);
        }
        break;

    case GameSituation::ConnectingToServer:
        if (std::holds_alternative<request::Connect>(request) &&
            std::get<request::Connect>(request) == request::Connect::Submit) {
            if (transport_.connectToServer("127.0.0.1", 5555)) {
                ui_.showMessage("Connected to server.");
                setSituation(GameSituation::ChoosingSide);
            } else {
                ui_.showMessage("Connection failed.");
                setSituation(GameSituation::MainMenu);
            }
        } else if (std::holds_alternative<request::Connect>(request) &&
                   std::get<request::Connect>(request) == request::Connect::Cancel) {
            setSituation(GameSituation::MainMenu);
        }
        break;

    case GameSituation::ChoosingSide:
        if (std::holds_alternative<request::SideSelection>(request) &&
            std::get<request::SideSelection>(request) == request::SideSelection::ChooseHumanity) {
            if (transport_.isConnected() && transport_.send(ClientPackage{ClientCommand::ChooseHumanity, ""})) {
                ui_.showMessage("Humanity selected.");
                setSituation(GameSituation::Game);
            } else {
                ui_.showMessage("Failed to notify server.");
                setSituation(GameSituation::MainMenu);
            }
        } else if (std::holds_alternative<request::SideSelection>(request) &&
                   std::get<request::SideSelection>(request) == request::SideSelection::ChoosePathogen) {
            if (transport_.isConnected() && transport_.send(ClientPackage{ClientCommand::ChoosePathogen, ""})) {
                ui_.showMessage("Pathogen selected.");
                setSituation(GameSituation::Game);
            } else {
                ui_.showMessage("Failed to notify server.");
                setSituation(GameSituation::MainMenu);
            }
        } else if (std::holds_alternative<request::SideSelection>(request) &&
                   std::get<request::SideSelection>(request) == request::SideSelection::Disconnect) {
            transport_.disconnect();
            ui_.showMessage("Disconnected.");
            setSituation(GameSituation::MainMenu);
        }
        break;

    case GameSituation::Game:
        if (std::holds_alternative<request::Game>(request) &&
            std::get<request::Game>(request) == request::Game::Leave) {
            setSituation(GameSituation::EndScreen);
        }
        break;

    case GameSituation::EndScreen:
        if (std::holds_alternative<request::EndScreen>(request) &&
            std::get<request::EndScreen>(request) == request::EndScreen::BackToMainMenu) {
            setSituation(GameSituation::MainMenu);
        }
        break;

    case GameSituation::Exiting:
        running_ = false;
        break;
    }
}

}
