#include "PlagueServer.hpp"
#include <sstream>
#include <iostream>
#include <vector>
#include <algorithm>

namespace plague {

PlagueServer::PlagueServer(const std::string& ip, int port) {
    uv_loop_init(&loop_);

    uv_tcp_init(&loop_, &server_);

    struct sockaddr_in addr;
    uv_ip4_addr(ip.c_str(), port, &addr);

    uv_tcp_bind(&server_, (const struct sockaddr*)&addr, 0);
    int r = uv_listen((uv_stream_t*)&server_, SOMAXCONN, on_new_connection);
    if (r) {
        std::cerr << "Listen error: " << uv_strerror(r) << std::endl;
        exit(1);
    }

    server_.data = this;
}

PlagueServer::~PlagueServer() {
    uv_close((uv_handle_t*)&server_, nullptr);
    uv_loop_close(&loop_);
}

void PlagueServer::run() {
    std::cout << "Server listening..." << std::endl;
    uv_run(&loop_, UV_RUN_DEFAULT);
}


void PlagueServer::on_new_connection(uv_stream_t* server, int status) {
    if (status < 0) {
        std::cerr << "New connection error: " << uv_strerror(status) << std::endl;
        return;
    }

    PlagueServer* self = static_cast<PlagueServer*>(server->data);
    uv_tcp_t* client = new uv_tcp_t;
    uv_tcp_init(&self->loop_, client);

    if (uv_accept(server, (uv_stream_t*)client) == 0) {
        client->data = client; 
        self->clients_.emplace(client, std::make_unique<ClientSession>(client));
        std::cout << "New connection" << std::endl;
        uv_read_start((uv_stream_t*)client, alloc_buffer, on_read);
    } else {
        uv_close((uv_handle_t*)client, on_close);
    }
}

void PlagueServer::alloc_buffer(uv_handle_t* /*handle*/, size_t suggested_size, uv_buf_t* buf) {
    buf->base = new char[suggested_size];
    buf->len = suggested_size;
}

void PlagueServer::on_read(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
    uv_tcp_t* client = reinterpret_cast<uv_tcp_t*>(stream);
    PlagueServer* self = static_cast<PlagueServer*>(client->data);

    if (nread < 0) {
        uv_read_stop(stream);
        return;
    }

    if (nread == 0) {
        delete[] buf->base;
        return;
    }

    auto it = self->clients_.find(client);
    if (it == self->clients_.end()) {
        delete[] buf->base;
        return;
    }

    ClientSession* session = it->second.get();
    session->readBuffer.append(buf->base, nread);
    delete[] buf->base;
}

void PlagueServer::on_close(uv_handle_t* handle) {
    uv_tcp_t* client = reinterpret_cast<uv_tcp_t*>(handle);
    delete client;
}

void PlagueServer::on_write_end(uv_write_t* req, int status) {
    if (status < 0) {
        std::cerr << "Write error: " << uv_strerror(status) << std::endl;
    }
    delete[] (char*)req->data; 
    delete req;
}


void PlagueServer::processInput(ClientSession* session, const std::string& line) {
    size_t space_pos = line.find(' ');
    if (space_pos == std::string::npos) {
        std::cerr << "Invalid format: " << line << std::endl;
        return;
    }

    std::string command_str = line.substr(0, space_pos);
    std::string payload_part = line.substr(space_pos + 1);

    size_t pipe_pos = payload_part.find('|');
    uint32_t request_id = 0;
    std::string extra_payload;
    if (pipe_pos != std::string::npos) {
        request_id = std::stoul(payload_part.substr(0, pipe_pos));
        extra_payload = payload_part.substr(pipe_pos + 1);
    } else {
        try {
            request_id = std::stoul(payload_part);
        } catch (...) {
            request_id = 0;
        }
    }

    ServerResponse response;
    response.request_id = request_id;
    response.success = true;

    if (command_str == "Connect") {
        response.payload = R"({"screen":"ChoosingSide"})"; 
    } else if (command_str == "Disconnect") {
        response.success = true;
        response.payload = "disconnected";
        sendResponse(session, response);
        uv_close((uv_handle_t*)session->socket, on_close);
        return;
    } else if (command_str == "ChooseHumanity") {
        session->role = PlayerRole::Humanity;
        response.payload = R"({"role":"humanity","screen":"Game","day":1,"points":100,"news":[]})";
    } else if (command_str == "ChoosePathogen") {
        session->role = PlayerRole::Pathogen;
        response.payload = R"({"role":"pathogen","screen":"Game","day":1,"points":100,"news":[]})";
    } else if (command_str == "Ping") {
        response.payload = "pong";
    } else {
        response.success = false;
        response.error_message = "unknown command";
    }

    sendResponse(session, response);
}

void PlagueServer::sendResponse(ClientSession* session, const ServerResponse& response) {
    std::ostringstream oss;
    oss << "RESPONSE " << response.request_id << "|" 
        << (response.success ? "1" : "0") << "|"
        << response.payload << "|"
        << response.error_message << "\n";

    std::string resp_str = oss.str();
    uv_buf_t buf = uv_buf_init(new char[resp_str.size() + 1], resp_str.size());
    std::copy(resp_str.begin(), resp_str.end(), buf.base);
    buf.base[resp_str.size()] = '\0'; 

    uv_write_t* req = new uv_write_t;
    req->data = buf.base; 
    uv_write(req, (uv_stream_t*)session->socket, &buf, 1, on_write_end);
}

} 