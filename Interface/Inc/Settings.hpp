#pragma once

#include <map>
#include <string>

namespace plague::ui {

/**
 * @brief Supported terminal layout profiles.
 *
 * The UI uses these profiles to choose map size, widget spacing, and the
 * terminal dimensions that are comfortable for the ncurses screens.
 */
enum class Resolutions {
    Low,
    Medium,
    High
};

/**
 * @brief Terminal dimensions required by a layout profile.
 */
struct TerminalProfile {
    /** Terminal height in rows. */
    int height, width;
};

/** @brief Minimum dimensions for every supported resolution profile. */
inline const std::map<Resolutions, TerminalProfile> terminalProfiles = {
    {Resolutions::Low, {34, 132}},
    {Resolutions::Medium, {47, 171}},
    {Resolutions::High, {76, 286}}
};

/**
 * @brief Logical screens managed by the interface layer.
 */
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
    News,
    End,
    ConnectionFailed
};

/**
 * @brief Mutable UI configuration shared by screens and the screen manager.
 */
struct Config {
    /** Currently selected terminal resolution profile. */
    Resolutions resolution = Resolutions::Medium;
    /** Current logical screen id. */
    ScreenIds id = ScreenIds::MainMenu;
    /** True when the terminal is below the minimum supported size. */
    bool terminalTooSmall = false;
    /** Last address entered on the connection screen, used by retry flows. */
    std::string lastConnectAddr{};
    /** Last port entered on the connection screen, used by retry flows. */
    std::string lastConnectPort{};
};

}
