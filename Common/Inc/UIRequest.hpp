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

using UIRequest = std::variant<
    None,
    MainMenu,
    Settings
>;

}  // namespace plague::request