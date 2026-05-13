#pragma once

#include "Client_ServerAPI.hpp"
#include "ClientPackage.hpp"
#include "SocketTransport.hpp"

#include <chrono>
#include <functional>
#include <mutex>
#include <queue>
#include <string>
#include <unordered_map>

namespace plague {

using ResponseCallback = std::function<void(const ServerResponse&)>;
using TimeoutCallback = std::function<void(RequestId)>;
using UnhandledResponseCallback = std::function<void(const ServerResponse&)>;

struct RequestConfig {
    std::chrono::milliseconds timeout{5000};
    int max_retries{2};
};

class RequestHandler {
public:
    RequestHandler(SocketTransport& transport);
    RequestId sendRequest(ClientCommand command,
                          std::string payload,
                          ResponseCallback on_response,
                          TimeoutCallback on_timeout = nullptr,
                          RequestConfig config = {});
    void handleIncoming(const ServerResponse& response);
    void update();
    void cancelRequest(RequestId id);
    bool hasPendingRequests() const;
    void setUnhandledResponseCallback(UnhandledResponseCallback callback);

private:
    struct PendingRequest {
        RequestId id;
        ClientCommand command;
        std::string payload;
        ResponseCallback on_response;
        TimeoutCallback on_timeout;
        std::chrono::steady_clock::time_point deadline;
        int retries_left;
        RequestConfig config;
    };

    RequestId generateRequestId();
    void enqueueOutgoing(RequestId id, ClientCommand command, const std::string& payload);
    void flushOutboundQueue();
    void processTimeout(const PendingRequest& req);

private:
    SocketTransport& transport_;
    RequestId next_request_id_{1};
    std::queue<ClientPackage> outbound_queue_;
    std::unordered_map<RequestId, PendingRequest> pending_requests_;
    UnhandledResponseCallback unhandled_response_callback_;
    mutable std::mutex mutex_;
};

}
