#include "RequestHandler.hpp"

#include "logger.hpp"

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

    // Сохраняем запрос до ответа сервера, чтобы связать пакет с callback по request_id.
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

    LOG_INFO("Sending request: id=%u command=%s",
             req_id,
             clientCommandToString(command));
    if (!payload.empty()) {
        LOG_DEBUG("Request payload: id=%u payload=%s", req_id, payload.c_str());
    }

    enqueueOutgoing(req_id, command, payload);
    flushOutboundQueue();
    return req_id;
}

void RequestHandler::handleIncoming(const ServerResponse& response) {
    ResponseCallback callback;
    UnhandledResponseCallback unhandled_callback;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto it = pending_requests_.find(response.request_id);
        if (it == pending_requests_.end()) {
            // Пуши от сервера приходят с последним id клиента и не всегда имеют pending-запрос.
            LOG_DEBUG("Received unhandled response: id=%u success=%d",
                      response.request_id,
                      response.success ? 1 : 0);
            unhandled_callback = unhandled_response_callback_;
        } else {
            LOG_INFO("Received response: id=%u success=%d",
                     response.request_id,
                     response.success ? 1 : 0);
            callback = std::move(it->second.on_response);
            pending_requests_.erase(it);
        }
    }

    // Callback может дернуть RequestHandler повторно, поэтому вызываем его уже без mutex.
    if (callback) {
        callback(response);
    } else if (unhandled_callback) {
        unhandled_callback(response);
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

    // Ретраи ставятся отдельным проходом, чтобы не держать mutex во время пользовательских callbacks.
    for (const PendingRequest& request : expired_requests) {
        processTimeout(request);
    }
}

void RequestHandler::cancelRequest(RequestId id) {
    std::lock_guard<std::mutex> lock(mutex_);
    LOG_INFO("Canceling request: id=%u", id);
    pending_requests_.erase(id);
}

bool RequestHandler::hasPendingRequests() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return !pending_requests_.empty();
}

void RequestHandler::setUnhandledResponseCallback(UnhandledResponseCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    unhandled_response_callback_ = std::move(callback);
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
        LOG_WARNING("Request timed out, retrying: id=%u command=%s retries_left=%d",
                    req.id,
                    clientCommandToString(req.command),
                    req.retries_left);
        PendingRequest retried_request = req;
        retried_request.retries_left -= 1;
        retried_request.deadline = std::chrono::steady_clock::now() + retried_request.config.timeout;

        // Повтор использует тот же request_id, чтобы поздний ответ всё ещё попал в исходный callback.
        std::lock_guard<std::mutex> lock(mutex_);
        pending_requests_[retried_request.id] = retried_request;
        enqueueOutgoing(retried_request.id, retried_request.command, retried_request.payload);
        flushOutboundQueue();
        return;
    }

    LOG_ERROR("Request timed out permanently: id=%u command=%s",
              req.id,
              clientCommandToString(req.command));
    if (req.on_timeout) {
        req.on_timeout(req.id);
    }
}

}
