#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace plague {

using RequestId = std::uint32_t;

enum class ClientCommand {
    Connect,
    Disconnect,
    ListRooms,
    CreateRoom,
    JoinRoom,
    ChooseHumanity,
    ChoosePathogen,
    SelectSubtype,
    ChangeSide,
    Ready,
    SelectCountry,
    Ping
};

struct ClientRequest {
    RequestId request_id{};
    ClientCommand command{};
    std::string payload{};
};

struct ServerResponse {
    RequestId request_id{};
    bool success{false};
    std::string payload{};
    std::string error_message{};
};

inline const char* clientCommandToString(ClientCommand command) {
    switch (command) {
    case ClientCommand::Connect:
        return "Connect";
    case ClientCommand::Disconnect:
        return "Disconnect";
    case ClientCommand::ListRooms:
        return "ListRooms";
    case ClientCommand::CreateRoom:
        return "CreateRoom";
    case ClientCommand::JoinRoom:
        return "JoinRoom";
    case ClientCommand::ChooseHumanity:
        return "ChooseHumanity";
    case ClientCommand::ChoosePathogen:
        return "ChoosePathogen";
    case ClientCommand::SelectSubtype:
        return "SelectSubtype";
    case ClientCommand::ChangeSide:
        return "ChangeSide";
    case ClientCommand::Ready:
        return "Ready";
    case ClientCommand::SelectCountry:
        return "SelectCountry";
    case ClientCommand::Ping:
        return "Ping";
    }

    return "Ping";
}

inline std::optional<ClientCommand> parseClientCommand(std::string_view text) {
    if (text == "Connect") {
        return ClientCommand::Connect;
    }
    if (text == "Disconnect") {
        return ClientCommand::Disconnect;
    }
    if (text == "ListRooms") {
        return ClientCommand::ListRooms;
    }
    if (text == "CreateRoom") {
        return ClientCommand::CreateRoom;
    }
    if (text == "JoinRoom") {
        return ClientCommand::JoinRoom;
    }
    if (text == "ChooseHumanity") {
        return ClientCommand::ChooseHumanity;
    }
    if (text == "ChoosePathogen") {
        return ClientCommand::ChoosePathogen;
    }
    if (text == "SelectSubtype") {
        return ClientCommand::SelectSubtype;
    }
    if (text == "ChangeSide") {
        return ClientCommand::ChangeSide;
    }
    if (text == "Ready") {
        return ClientCommand::Ready;
    }
    if (text == "SelectCountry") {
        return ClientCommand::SelectCountry;
    }
    if (text == "Ping") {
        return ClientCommand::Ping;
    }

    return std::nullopt;
}

inline std::optional<RequestId> parseRequestId(std::string_view text) {
    if (text.empty()) {
        return std::nullopt;
    }

    try {
        return static_cast<RequestId>(std::stoul(std::string(text)));
    } catch (...) {
        return std::nullopt;
    }
}

inline std::string serializeClientPackage(ClientCommand command, std::string_view payload) {
    std::string wire = clientCommandToString(command);
    wire.push_back(' ');
    wire.append(payload);
    wire.push_back('\n');
    return wire;
}

inline std::optional<ClientRequest> parseClientRequestLine(std::string_view line) {
    const std::size_t space_pos = line.find(' ');
    if (space_pos == std::string_view::npos) {
        return std::nullopt;
    }

    const auto command = parseClientCommand(line.substr(0, space_pos));
    if (!command.has_value()) {
        return std::nullopt;
    }

    const std::string_view payload_part = line.substr(space_pos + 1);
    const std::size_t pipe_pos = payload_part.find('|');

    const std::string_view request_id_text = pipe_pos == std::string_view::npos
        ? payload_part
        : payload_part.substr(0, pipe_pos);

    const auto request_id = parseRequestId(request_id_text);
    if (!request_id.has_value()) {
        return std::nullopt;
    }

    ClientRequest request;
    request.request_id = *request_id;
    request.command = *command;
    if (pipe_pos != std::string_view::npos) {
        request.payload = std::string(payload_part.substr(pipe_pos + 1));
    }

    return request;
}

inline std::string serializeServerResponse(const ServerResponse& response) {
    return "RESPONSE " + std::to_string(response.request_id) + "|" +
           (response.success ? "1" : "0") + "|" +
           response.payload + "|" +
           response.error_message + "\n";
}

inline std::optional<ServerResponse> parseServerResponseLine(std::string_view line) {
    constexpr std::string_view prefix = "RESPONSE ";
    if (!line.starts_with(prefix)) {
        return std::nullopt;
    }

    const std::string_view body = line.substr(prefix.size());
    const std::size_t first_pipe = body.find('|');
    const std::size_t second_pipe = first_pipe == std::string_view::npos ? std::string_view::npos : body.find('|', first_pipe + 1);
    const std::size_t third_pipe = second_pipe == std::string_view::npos ? std::string_view::npos : body.find('|', second_pipe + 1);

    if (first_pipe == std::string_view::npos ||
        second_pipe == std::string_view::npos ||
        third_pipe == std::string_view::npos) {
        return std::nullopt;
    }

    const auto request_id = parseRequestId(body.substr(0, first_pipe));
    if (!request_id.has_value()) {
        return std::nullopt;
    }

    const std::string_view success_text = body.substr(first_pipe + 1, second_pipe - first_pipe - 1);
    if (success_text != "0" && success_text != "1") {
        return std::nullopt;
    }

    ServerResponse response;
    response.request_id = *request_id;
    response.success = success_text == "1";
    response.payload = std::string(body.substr(second_pipe + 1, third_pipe - second_pipe - 1));
    response.error_message = std::string(body.substr(third_pipe + 1));
    return response;
}

}
