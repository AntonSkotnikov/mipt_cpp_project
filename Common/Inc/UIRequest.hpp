#pragma once

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

using UIRequest = std::variant<
    None,
    MainMenu,
    Settings,
    Connect
>;

}  // namespace plague::request
