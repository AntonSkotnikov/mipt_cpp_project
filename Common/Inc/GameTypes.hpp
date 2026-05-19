#pragma once

#include <cstdint>
#include <variant>

namespace plague {

using UpgradePointType = std::uint16_t;

enum class PlayerRole {
    Humanity,
    Pathogen
};

enum class HumanitySubtype {
    ResearchInstitute
};

enum class PathogenSubtype {
    Virus
};

using PlayerSubtype = std::variant<HumanitySubtype, PathogenSubtype>;

enum class ChoosingSideSignal {
    None,
    LocalReady,
    OpponentReady,
    OpponentRequestsSideChange
};

enum class GameSituation {
    MainMenu,

    Settings,
    ConnectToServer,
    Exit,
    Exiting = Exit,

    ConnectingToServer,
    ConnectingToServerFailed,

    RoomBrowser,
    ChoosingSide,
    Game,
    EndScreen,
};

// Humanity Upgrades

enum class HumanityAbilities {

};

enum class VaccineAbilities {

};

enum class AntiSpreadAbilities {

};

// Pathogen Upgrades

enum class SpreadAbilities {

};

enum class SymptomsAbilities {

};

enum class PathogenAbilities {

};

struct InfoAboutPlayer {
    PlayerRole       role;
    UpgradePointType points;
    PlayerSubtype    subtype = HumanitySubtype::ResearchInstitute;
};

struct ChoosingSideState {
    PlayerRole predefinedRole = PlayerRole::Humanity;
    PlayerSubtype selectedSubtype = HumanitySubtype::ResearchInstitute;
    bool sideChangeRequested = false;
    bool ready = false;
    bool opponentSideChangeRequested = false;
    bool opponentReady = false;
    ChoosingSideSignal signal = ChoosingSideSignal::None;
};

}
