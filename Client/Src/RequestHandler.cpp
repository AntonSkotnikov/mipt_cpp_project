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
