#include "ClientApp.hpp"

#include <chrono>
#include <thread>

namespace plague {

namespace {

constexpr const char* kServerHost = "127.0.0.1";
constexpr int kServerPort = 5555;

}

ClientApp::ClientApp(IUserInterface& ui, ITransport& transport)
    : ui_(ui), transport_(transport), request_handler_(std::make_unique<RequestHandler>(transport)) {}

void ClientApp::run() {
    using clock = std::chrono::steady_clock;
    constexpr auto frameTime = std::chrono::microseconds(16667);
    auto nextFrame = clock::now();

    while (running_) {
        nextFrame += frameTime;

        ui_.render(snapshot_);
        const request::UIRequest request = ui_.pollRequest(snapshot_);
        handleRequest(request);
        request_handler_->update();

        std::this_thread::sleep_until(nextFrame);
        if (clock::now() > nextFrame + frameTime) {
            nextFrame = clock::now();
        }
    }
}

void ClientApp::setSituation(GameSituation newSituation) {
    snapshot_.situation = newSituation;
}

void ClientApp::resetSnapshotForMenu() {
    snapshot_.day = 0;
    snapshot_.playerInfo.points = 0;
    snapshot_.recentNews.clear();
}

void ClientApp::handleRequest(const request::UIRequest& request) {
    switch (snapshot_.situation) {
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
        if (std::holds_alternative<request::ConnectInfo>(request) &&
            std::get<request::ConnectInfo>(request).id == request::Connect::Connect) {
            if (request_handler_->hasPendingRequests()) {
                break;
            }

            const auto& info = std::get<request::ConnectInfo>(request);
            std::string host = info.addr.empty() ? kServerHost : info.addr;
            int port = kServerPort;
            if (!info.port.empty()) {
                try {
                    port = std::stoi(info.port);
                } catch (...) {}
            }

            if (!transport_.isConnected() && !transport_.connectToServer(host.c_str(), port)) {
                ui_.showMessage("Connection failed.");
                resetSnapshotForMenu();
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
                        snapshot_.recentNews.clear();
                        setSituation(GameSituation::ChoosingSide);
                    } else {
                        ui_.showMessage(response.error_message.empty() ? "Connection failed." : response.error_message.c_str());
                        transport_.disconnect();
                        resetSnapshotForMenu();
                        setSituation(GameSituation::MainMenu);
                    }
                },
                [this](RequestId) {
                    ui_.showMessage("Connection timeout.");
                    transport_.disconnect();
                    resetSnapshotForMenu();
                    setSituation(GameSituation::MainMenu);
                });
        } else if (std::holds_alternative<request::ConnectInfo>(request) &&
                   std::get<request::ConnectInfo>(request).id == request::Connect::Back) {
            setSituation(GameSituation::MainMenu);
            ui_.showMessage("");
        }
        break;

    case GameSituation::ConnectingToServerFailed:
        resetSnapshotForMenu();
        setSituation(GameSituation::MainMenu);
        break;

    case GameSituation::ChoosingSide:
        if (request_handler_->hasPendingRequests()) {
            break;
        }

        if (std::holds_alternative<request::MainMenu>(request)) {
            auto action = std::get<request::MainMenu>(request);
            if (action == request::MainMenu::ConnectToServer) {
                ui_.showMessage("Sending humanity choice...");
                request_handler_->sendRequest(
                    ClientCommand::ChooseHumanity,
                    "",
                    [this](const ServerResponse& response) {
                        if (response.success) {
                            ui_.showMessage("Humanity selected.");
                            snapshot_.playerInfo.role = PlayerRole::Humanity;
                            snapshot_.playerInfo.points = 100;
                            snapshot_.day = 1;
                            snapshot_.recentNews.emplace_back(ImportanceOfNews::RegularNews, "Humanity side selected.");
                            setSituation(GameSituation::Game);
                        } else {
                            ui_.showMessage(response.error_message.empty() ? "Failed to notify server." : response.error_message.c_str());
                            resetSnapshotForMenu();
                            setSituation(GameSituation::MainMenu);
                        }
                    },
                    [this](RequestId) {
                        ui_.showMessage("Server timeout. Try again.");
                        resetSnapshotForMenu();
                        setSituation(GameSituation::MainMenu);
                    });
            } else if (action == request::MainMenu::OpenSettings) {
                ui_.showMessage("Sending pathogen choice...");
                request_handler_->sendRequest(
                    ClientCommand::ChoosePathogen,
                    "",
                    [this](const ServerResponse& response) {
                        if (response.success) {
                            ui_.showMessage("Pathogen selected.");
                            snapshot_.playerInfo.role = PlayerRole::Pathogen;
                            snapshot_.playerInfo.points = 100;
                            snapshot_.day = 1;
                            snapshot_.recentNews.emplace_back(ImportanceOfNews::RegularNews, "Pathogen side selected.");
                            setSituation(GameSituation::Game);
                        } else {
                            ui_.showMessage(response.error_message.empty() ? "Failed to notify server." : response.error_message.c_str());
                            resetSnapshotForMenu();
                            setSituation(GameSituation::MainMenu);
                        }
                    },
                    [this](RequestId) {
                        ui_.showMessage("Server timeout. Try again.");
                        resetSnapshotForMenu();
                        setSituation(GameSituation::MainMenu);
                    });
            } else if (action == request::MainMenu::Exit) {
                transport_.disconnect();
                ui_.showMessage("Disconnected.");
                resetSnapshotForMenu();
                setSituation(GameSituation::MainMenu);
            }
        }
        break;

    case GameSituation::Game:
        if (std::holds_alternative<request::Settings>(request) &&
            std::get<request::Settings>(request) == request::Settings::Back) {
            snapshot_.recentNews.emplace_back(ImportanceOfNews::RegularNews, "Left current game.");
            setSituation(GameSituation::EndScreen);
        }
        break;

    case GameSituation::EndScreen:
        if (std::holds_alternative<request::Settings>(request) &&
            std::get<request::Settings>(request) == request::Settings::Back) {
            resetSnapshotForMenu();
            setSituation(GameSituation::MainMenu);
        }
        break;

    case GameSituation::Exiting:
        running_ = false;
        break;
    }
}

}
