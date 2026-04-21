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
