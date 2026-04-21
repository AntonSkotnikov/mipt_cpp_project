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
    Submit,
    Cancel
};

enum class SideSelection {
    ChooseHumanity,
    ChoosePathogen,
    Disconnect
};

enum class Game {
    Leave
};

enum class EndScreen {
    BackToMainMenu
};

using UIRequest = std::variant<
    None,
    MainMenu,
    Settings,
    Connect,
    SideSelection,
    Game,
    EndScreen
>;

}  // namespace plague::request
