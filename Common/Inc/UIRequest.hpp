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

using UIRequest = std::variant<
    None,
    MainMenu,
    Settings,
    ConnectInfo,
    ChoosingSide,
    Game
>;

}  // namespace plague::request
