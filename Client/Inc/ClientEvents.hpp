#pragma once

namespace plague {

enum class ClientEvent {
    None,

    // Main menu
    GoToConnectMenu,
    GoToSettings,
    ExitRequested,

    // Connecting screen
    SubmitConnect,
    CancelConnect,
    DisconnectRequested,

    // Side selection
    ChooseHumanity,
    ChoosePathogen,
    BackToMainMenu,

    // Game / end flow
    LeaveGame
};

}
