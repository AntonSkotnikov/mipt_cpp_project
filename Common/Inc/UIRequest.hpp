#pragma once

#include <string>
#include <variant>

namespace plague::request {

struct None {};

enum class MainMenu {
    ConnectToServer,
    OpenSettings,
    Exit
};

enum class Settings {
    Back
};

enum class Connect {
    Back,
    Connect
};

enum class RoomAction {
    Back,
    Join,
    Create
};

struct RoomRequest {
    RoomAction action;
    std::string roomName;
    std::string password;
};

enum class ChoosingSideAction {
    SelectSubtype,
    ChangeSide,
    Ready
};

struct ChoosingSide {
    ChoosingSideAction action;
    int subtypeIndex = 0;
};

enum class Game {
    Upgrade,
    Info,
    Transmission,
    Clinic,
    Abilities,
    World,
    Cure,
    News,
    Back
};

struct ConnectInfo {
    Connect id;
    std::string addr;
    std::string port;
};

struct SelectCountry {
    std::string countryName;
};

using UIRequest = std::variant<
    None,
    MainMenu,
    Settings,
    ConnectInfo,
    RoomRequest,
    ChoosingSide,
    Game,
    SelectCountry
>;

}  // namespace plague::request
