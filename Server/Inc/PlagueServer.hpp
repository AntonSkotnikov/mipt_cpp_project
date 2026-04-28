#pragma once

#include <uv.h>
#include <string>
#include <unordered_map>
#include <memory>
#include <cstdint>

namespace plague {

// Временное определение PlayerRole (должно совпадать с GameTypes.hpp)
enum class PlayerRole { Humanity, Pathogen };

struct ServerResponse {
    uint32_t request_id;
    bool success;
    std::string payload;
    std::string error_message;
};

struct ClientSession {
    uv_tcp_t* socket;
    std::string readBuffer;
    PlayerRole role = PlayerRole::Humanity; 
    int points = 100;
    explicit ClientSession(uv_tcp_t* s) : socket(s) {}
};

class PlagueServer {
public:
    PlagueServer(const std::string& ip, int port);
    ~PlagueServer();
    void run();

private:
    static void on_new_connection(uv_stream_t* server, int status);
    static void alloc_buffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf);
    static void on_read(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf);
    static void on_close(uv_handle_t* handle);
    static void on_write_end(uv_write_t* req, int status);

    void processInput(ClientSession* session, const std::string& line);
    void sendResponse(ClientSession* session, const ServerResponse& response);

    uv_loop_t loop_;
    uv_tcp_t server_;
    std::unordered_map<uv_tcp_t*, std::unique_ptr<ClientSession>> clients_;
};

} // namespace plague