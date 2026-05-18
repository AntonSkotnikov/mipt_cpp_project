#pragma once

#include <map>

namespace plague::ui {

enum class Resolutions {
    Low,
    Medium,
    High
};

struct TerminalProfile {
    int height, width;
};

inline const std::map<Resolutions, TerminalProfile> terminalProfiles = {
    {Resolutions::Low, {34, 132}},
    {Resolutions::Medium, {47, 171}},
    {Resolutions::High, {76, 286}}
};

enum class ScreenIds {
    SmallTerm,
    MainMenu,
    Connect,
    Rooms,
    Settings,
    Game,
    Info,
    Transmission,
    Clinic,
    Abilities,
    World,
    Cure,
    News
};

struct Config {
    Resolutions resolution = Resolutions::Medium;
    ScreenIds id = ScreenIds::MainMenu;
    bool terminalTooSmall = false;
};

}
