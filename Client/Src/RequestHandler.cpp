#include "RequestHandler.hpp"

#include <vector>

namespace plague {

RequestHandler::RequestHandler(SocketTransport& transport)
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
        payload,
        std::move(on_response),
        std::move(on_timeout),
        deadline,
        config.max_retries,
        config
    });

    enqueueOutgoing(req_id, command, payload);
    flushOutboundQueue();
    return req_id;
}

void RequestHandler::handleIncoming(const ServerResponse& response) {
    ResponseCallback callback;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto it = pending_requests_.find(response.request_id);
        if (it == pending_requests_.end()) {
            return;
        }

        callback = std::move(it->second.on_response);
        pending_requests_.erase(it);
    }

    if (callback) {
        callback(response);
    }
}

void RequestHandler::update() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        flushOutboundQueue();
    }

    ServerResponse response;
    while (transport_.pollResponse(response)) {
        handleIncoming(response);
    }

    std::vector<PendingRequest> expired_requests;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto now = std::chrono::steady_clock::now();

        for (auto it = pending_requests_.begin(); it != pending_requests_.end();) {
            if (now >= it->second.deadline) {
                expired_requests.push_back(it->second);
                it = pending_requests_.erase(it);
            } else {
                ++it;
            }
        }
    }

    for (const PendingRequest& request : expired_requests) {
        processTimeout(request);
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

void RequestHandler::enqueueOutgoing(RequestId id, ClientCommand command, const std::string& payload) {
    outbound_queue_.push(ClientPackage{command, std::to_string(id) + "|" + payload});
}

void RequestHandler::flushOutboundQueue() {
    if (!transport_.isConnected()) {
        return;
    }

    while (!outbound_queue_.empty()) {
        if (!transport_.send(outbound_queue_.front())) {
            break;
        }
        outbound_queue_.pop();
    }
}

void RequestHandler::processTimeout(const PendingRequest& req) {
    if (req.retries_left > 0 && transport_.isConnected()) {
        PendingRequest retried_request = req;
        retried_request.retries_left -= 1;
        retried_request.deadline = std::chrono::steady_clock::now() + retried_request.config.timeout;

        std::lock_guard<std::mutex> lock(mutex_);
        pending_requests_[retried_request.id] = retried_request;
        enqueueOutgoing(retried_request.id, retried_request.command, retried_request.payload);
        flushOutboundQueue();
        return;
    }

    if (req.on_timeout) {
        req.on_timeout(req.id);
    }
}

}
