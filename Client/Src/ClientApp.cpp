#include "ClientApp.hpp"

namespace plague {

namespace {

constexpr const char* kServerHost = "127.0.0.1";
constexpr int kServerPort = 5555;

}

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
            ui_.showMessage("");
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

    case GameSituation::ConnectToServer:
    case GameSituation::ConnectingToServer:
        if (std::holds_alternative<request::Connect>(request) &&
            std::get<request::Connect>(request) == request::Connect::Submit) {
            if (request_handler_->hasPendingRequests()) {
                break;
            }

            if (!transport_.isConnected() && !transport_.connectToServer(kServerHost, kServerPort)) {
                ui_.showMessage("Connection failed.");
                setSituation(GameSituation::MainMenu);
                break;
            }

            ui_.showMessage("Connecting to server...");
            request_handler_->sendRequest(
                ClientCommand::Connect,
                "",
                [this](const ServerResponse& response) {
                    if (response.success) {
                        ui_.showMessage("Connected to server.");
                        setSituation(GameSituation::ChoosingSide);
                    } else {
                        ui_.showMessage(response.error_message.empty() ? "Connection failed." : response.error_message.c_str());
                        transport_.disconnect();
                        setSituation(GameSituation::MainMenu);
                    }
                },
                [this](RequestId) {
                    ui_.showMessage("Connection timeout.");
                    transport_.disconnect();
                    setSituation(GameSituation::MainMenu);
                });
        } else if (std::holds_alternative<request::Connect>(request) &&
                   std::get<request::Connect>(request) == request::Connect::Cancel) {
            setSituation(GameSituation::MainMenu);
            ui_.showMessage("");
        }
        break;

    case GameSituation::ConnectingToServerFailed:
        setSituation(GameSituation::MainMenu);
        break;

    case GameSituation::ChoosingSide:
        if (request_handler_->hasPendingRequests()) {
            break;
        }

        if (std::holds_alternative<request::SideSelection>(request) &&
            std::get<request::SideSelection>(request) == request::SideSelection::ChooseHumanity) {
            ui_.showMessage("Sending humanity choice...");
            request_handler_->sendRequest(
                ClientCommand::ChooseHumanity,
                "",
                [this](const ServerResponse& response) {
                    if (response.success) {
                        ui_.showMessage("Humanity selected.");
                        setSituation(GameSituation::Game);
                    } else {
                        ui_.showMessage(response.error_message.empty() ? "Failed to notify server." : response.error_message.c_str());
                        setSituation(GameSituation::MainMenu);
                    }
                },
                [this](RequestId) {
                    ui_.showMessage("Server timeout. Try again.");
                    setSituation(GameSituation::MainMenu);
                });
        } else if (std::holds_alternative<request::SideSelection>(request) &&
                   std::get<request::SideSelection>(request) == request::SideSelection::ChoosePathogen) {
            ui_.showMessage("Sending pathogen choice...");
            request_handler_->sendRequest(
                ClientCommand::ChoosePathogen,
                "",
                [this](const ServerResponse& response) {
                    if (response.success) {
                        ui_.showMessage("Pathogen selected.");
                        setSituation(GameSituation::Game);
                    } else {
                        ui_.showMessage(response.error_message.empty() ? "Failed to notify server." : response.error_message.c_str());
                        setSituation(GameSituation::MainMenu);
                    }
                },
                [this](RequestId) {
                    ui_.showMessage("Server timeout. Try again.");
                    setSituation(GameSituation::MainMenu);
                });
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
