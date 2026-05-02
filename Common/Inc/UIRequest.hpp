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

struct ConnectInfo {
    Connect id;
    std::string addr;
    std::string port;
};

using UIRequest = std::variant<
    None,
    MainMenu,
    Settings,
    ConnectInfo
>;

}  // namespace plague::request
