#pragma once

#include "Screen.hpp"
#include "Settings.hpp"
#include "Window.hpp"

namespace plague::ui {

/**
 * @brief Owns all UI screens and the shared main window.
 *
 * ScreenManager keeps screen instances alive, applies the current terminal
 * layout, and selects the screen pointed to by Config::id.
 */
class ScreenManager final {
private:
    Config & cfg_;
    Window mainWin_; // for border if needed
public:
    /** Screen displayed when the terminal is too small. */
    SmallTermScreen smallTerm_;
    /** Dialog screen for failed server connection attempts. */
    ConnectionFailedScreen connectionFailed_;
    /** Main menu screen. */
    MainMenuScreen mainMenu_;
    /** Server connection form. */
    ConnectToServerScreen connect_;
    /** Room browser and room creation/join screen. */
    RoomBrowserScreen rooms_;
    /** Role and subtype selection screen. */
    ChoosingSideScreen choosingSide_;

    /** Main world map gameplay screen. */
    GameScreen game_;

    /** Pathogen/player information screen. */
    PathogenInfoScreen pathogen_;
    /** Transmission upgrades screen. */
    UpgradeScreen transmission_;
    /** Clinic upgrades screen. */
    UpgradeScreen clinic_;
    /** Abilities upgrades screen. */
    UpgradeScreen abilities_;
    /** Cure information screen. */
    CureInfoScreen cure_;
    /** Country list and country details screen. */
    CountryScreen country_;
    /** News log screen. */
    NewsScreen news_;
    /** Victory/defeat screen. */
    EndScreen end_;

    /** Currently active screen. */
    Screen * curScreen = &mainMenu_;
    /** @brief Construct all screens for a shared config. */
    ScreenManager(Config & cfg);
    /** @brief Resize the main window and all screens after terminal changes. */
    void resize();
private:
    void applyWindowLayout();
    void selectCurrentScreen();
};

}
