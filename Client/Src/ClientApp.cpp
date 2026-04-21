#pragma once

#include "ITransport.hpp"
#include "ClientPackage.hpp"
#include <functional>
#include <unordered_map>
#include <queue>
#include <chrono>
#include <mutex>
#include <string>
#include <cstdint>

namespace plague {

using RequestId = std::uint32_t;

struct ServerResponse {
    RequestId request_id{};
    bool success{false};
    std::string payload{};
    std::string error_message{};
};

using ResponseCallback = std::function<void(const ServerResponse&)>;
using TimeoutCallback = std::function<void(RequestId)>;

struct RequestConfig {
    std::chrono::milliseconds timeout{5000};
    int max_retries{2};
};

class RequestHandler {
public:
    RequestHandler(ITransport& transport);
    RequestId sendRequest(ClientCommand command,
                         std::string payload,
                         ResponseCallback on_response,
                         TimeoutCallback on_timeout = nullptr,
                         RequestConfig config = {});
    void handleIncoming(const ServerResponse& response);
    void update();
    void cancelRequest(RequestId id);
    bool hasPendingRequests() const;

private:
    struct PendingRequest {
        RequestId id;
        ClientCommand command;
        ResponseCallback on_response;
        TimeoutCallback on_timeout;
        std::chrono::steady_clock::time_point deadline;
        int retries_left;
        RequestConfig config;
    };

    RequestId generateRequestId();
    void processTimeout(const PendingRequest& req);

private:
    ITransport& transport_;
    RequestId next_request_id_{1};
    std::queue<ClientPackage> outbound_queue_;
    std::unordered_map<RequestId, PendingRequest> pending_requests_;
    mutable std::mutex mutex_;
};

}
```

### `Client/Src/RequestHandler.cpp`
```cpp
#include "RequestHandler.hpp"
#include <vector>

namespace plague {

RequestHandler::RequestHandler(ITransport& transport)
    : transport_(transport) {}

RequestId RequestHandler::generateRequestId() {
    return next_request_id_++;
}

RequestId RequestHandler::sendRequest(ClientCommand command,
                                      std::string payload,
                                      ResponseCallback on_response,
                                      TimeoutCallback on_timeout,
                                      RequestConfig config) {
    std::lock_guard<std::mutex> lock(mutex_);

    const RequestId req_id = generateRequestId();
    const auto deadline = std::chrono::steady_clock::now() + config.timeout;

    pending_requests_.emplace(req_id, PendingRequest{
        req_id,
        command,
        std::move(on_response),
        std::move(on_timeout),
        deadline,
        config.max_retries,
        config
    });

    std::string enriched_payload = std::to_string(req_id) + "|" + payload;
    outbound_queue_.push(ClientPackage{command, std::move(enriched_payload)});

    if (transport_.isConnected()) {
        while (!outbound_queue_.empty()) {
            if (transport_.send(outbound_queue_.front())) {
                outbound_queue_.pop();
            } else {
                break;
            }
        }
    }

    return req_id;
}

void RequestHandler::handleIncoming(const ServerResponse& response) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = pending_requests_.find(response.request_id);
    if (it == pending_requests_.end()) {
        return;
    }

    if (it->second.on_response) {
        it->second.on_response(response);
    }

    pending_requests_.erase(it);
}

void RequestHandler::update() {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto now = std::chrono::steady_clock::now();

    if (transport_.isConnected()) {
        while (!outbound_queue_.empty()) {
            if (transport_.send(outbound_queue_.front())) {
                outbound_queue_.pop();
            } else {
                break;
            }
        }
    }

    std::vector<RequestId> timed_out;
    for (const auto& pair : pending_requests_) {
        if (now >= pair.second.deadline) {
            timed_out.push_back(pair.first);
        }
    }

    for (RequestId id : timed_out) {
        auto it = pending_requests_.find(id);
        if (it != pending_requests_.end()) {
            PendingRequest req = std::move(it->second);
            pending_requests_.erase(it);
            processTimeout(req);
        }
    }
}

void RequestHandler::processTimeout(const PendingRequest& req) {
    if (req.retries_left > 0 && transport_.isConnected()) {
        std::string enriched_payload = std::to_string(req.id) + "|" + std::to_string(static_cast<int>(req.command));
        outbound_queue_.push(ClientPackage{req.command, std::move(enriched_payload)});
    } else if (req.on_timeout) {
        req.on_timeout(req.id);
    }
}

void RequestHandler::cancelRequest(RequestId id) {
    std::lock_guard<std::mutex> lock(mutex_);
    pending_requests_.erase(id);
}

bool RequestHandler::hasPendingRequests() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return !pending_requests_.empty();
}

}
```

### `Client/Inc/ClientApp.hpp`
```cpp
#pragma once

#include "GameTypes.hpp"
#include "ClientEvents.hpp"
#include "ITransport.hpp"
#include "IUserInterface.hpp"
#include "RequestHandler.hpp"
#include <memory>

namespace plague {

class ClientApp {
public:
    ClientApp(IUserInterface& ui, ITransport& transport);
    void run();

private:
    void handleEvent(ClientEvent event);
    void setSituation(GameSituation newSituation);

private:
    IUserInterface& ui_;
    ITransport& transport_;
    std::unique_ptr<RequestHandler> request_handler_;
    GameSituation situation_ = GameSituation::MainMenu;
    bool running_ = true;
};

}
```

### `Client/Src/ClientApp.cpp`
```cpp
#include "ClientApp.hpp"

namespace plague {

ClientApp::ClientApp(IUserInterface& ui, ITransport& transport)
    : ui_(ui), transport_(transport), request_handler_(std::make_unique<RequestHandler>(transport)) {}

void ClientApp::run() {
    while (running_) {
        ui_.render(situation_);
        const ClientEvent event = ui_.pollEvent(situation_);
        handleEvent(event);
        request_handler_->update();
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
            request_handler_->sendRequest(ClientCommand::Connect, "",
                [this](const ServerResponse& resp) {
                    if (resp.success && transport_.connectToServer("127.0.0.1", 5555)) {
                        ui_.showMessage("Connected to server.");
                        setSituation(GameSituation::ChoosingSide);
                    } else {
                        ui_.showMessage("Connection failed.");
                        setSituation(GameSituation::MainMenu);
                    }
                });
        } else if (event == ClientEvent::CancelConnect) {
            setSituation(GameSituation::MainMenu);
        }
        break;

    case GameSituation::ChoosingSide:
        if (event == ClientEvent::ChooseHumanity) {
            request_handler_->sendRequest(ClientCommand::ChooseHumanity, "",
                [this](const ServerResponse& resp) {
                    if (resp.success) {
                        ui_.showMessage("Humanity selected.");
                        setSituation(GameSituation::Game);
                    } else {
                        ui_.showMessage("Failed to notify server.");
                        setSituation(GameSituation::MainMenu);
                    }
                },
                [this](RequestId) {
                    ui_.showMessage("Server timeout. Try again.");
                    setSituation(GameSituation::MainMenu);
                });
        } else if (event == ClientEvent::ChoosePathogen) {
            request_handler_->sendRequest(ClientCommand::ChoosePathogen, "",
                [this](const ServerResponse& resp) {
                    if (resp.success) {
                        ui_.showMessage("Pathogen selected.");
                        setSituation(GameSituation::Game);
                    } else {
                        ui_.showMessage("Failed to notify server.");
                        setSituation(GameSituation::MainMenu);
                    }
                });
        } else if (event == ClientEvent::DisconnectRequested) {
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
