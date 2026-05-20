#pragma once

#include "Settings.hpp"
#include "UIRequest.hpp"
#include "UI_ClientAPI.hpp"
#include "Upgrade.hpp"
#include "Window.hpp"
#include "Widget.hpp"
#include <cstddef>
#include <memory>
#include <vector>

namespace plague::ui {

/**
 * @brief Base class for all full-screen UI states.
 *
 * A screen owns widgets, manages focus among them, draws the screen, and turns
 * ncurses key codes into request::UIRequest values.
 */
class Screen {
public:
    virtual ~Screen() = default;

    /** @brief Clear the window, draw all widgets, and refresh the terminal. */
    void draw();
    /**
     * @brief Handle one input key for the active screen.
     * @param key ncurses key code.
     * @return Request emitted by the screen, or request::None.
     */
    virtual request::UIRequest handleInput(int key) = 0;
    /** @brief Recompute screen layout after window/terminal size changes. */
    virtual void resize();
    /** @return Next input key from the shared window. */
    int getKey();
    /** @brief Construct a screen using shared UI config and window. */
    Screen(Config & cfg, Window & win);
protected:
    std::vector<std::unique_ptr<Widget>> widgets{};
    Config & cfg_;
    Window & win_;

    /** @brief Focus the first focusable widget, if any. */
    void focusFirst();
    /** @brief Focus a widget by index when it is focusable. */
    void focusWidget(std::size_t index);
    /** @brief Move focus to the next focusable widget. */
    void focusNext();
    /** @brief Move focus to the previous focusable widget. */
    void focusPrev();
    /** @return Currently focused widget, or nullptr when none is available. */
    Widget * focusedWidget();
    /** @brief Forward a key to the focused widget. */
    InputResult handleFocusedInput(int key);
    std::size_t focusedIndex_ = 0;
};

/** @brief Main menu screen with logo and top-level actions. */
class MainMenuScreen final: public Screen {
public:
    MainMenuScreen(Config & cfg, Window & win);

    request::UIRequest handleInput(int key) override;
    void resize() override;
private:
    void layout();
};

/** @brief Warning screen shown when the terminal is too small. */
class SmallTermScreen final: public Screen {
public:
    SmallTermScreen(Config & cfg, Window & win);

    request::UIRequest handleInput(int key) override;
    void resize() override;
private:
    void layout();
};

/** @brief Dialog screen shown after a failed server connection attempt. */
class ConnectionFailedScreen final : public Screen {
public:
    ConnectionFailedScreen(Config & cfg, Window & win);

    request::UIRequest handleInput(int key) override;
    void resize() override;
private:
    void layout();
};

/** @brief Server address/port input form. */
class ConnectToServerScreen final : public Screen {
public:
    ConnectToServerScreen(Config & cfg, Window & win);

    request::UIRequest handleInput(int key) override;
    void resize() override;
private:
	    void layout();
};

/** @brief Room browser with join/create/back controls. */
class RoomBrowserScreen final : public Screen {
public:
    RoomBrowserScreen(Config & cfg, Window & win);

    request::UIRequest handleInput(int key) override;
    void resize() override;
    /** @brief Update displayed rooms and status from a snapshot. */
    void updateSnapshot(const GameSnapshot & snapshot);
private:
    static constexpr std::size_t roomListIndex_ = 0;
    static constexpr std::size_t selectedPasswordIndex_ = 1;
    static constexpr std::size_t createNameIndex_ = 2;
    static constexpr std::size_t createPasswordIndex_ = 3;
    static constexpr std::size_t statusIndex_ = 4;
    static constexpr std::size_t joinButtonIndex_ = 5;
    static constexpr std::size_t createButtonIndex_ = 6;
    static constexpr std::size_t backButtonIndex_ = 7;

    RoomList * roomList_ = nullptr;
    TextInput * selectedPassword_ = nullptr;
    TextInput * createName_ = nullptr;
    TextInput * createPassword_ = nullptr;
    VariableInfo * status_ = nullptr;
    GameSnapshot snapshot_{};
    bool roomNavigationMode_ = false;

    void layout();
    void updateRooms();
    void updateStatus();
    request::UIRequest joinSelectedRoom() const;
    request::UIRequest createRoom() const;
    void focusNextField();
    void focusPrevField();
};

/** @brief Role/subtype selection lobby screen. */
class ChoosingSideScreen final : public Screen {
public:
    ChoosingSideScreen(Config & cfg, Window & win);

    request::UIRequest handleInput(int key) override;
    void resize() override;
    /** @brief Update role, subtype, readiness, and status text. */
    void updateSnapshot(const GameSnapshot & snapshot);
private:
    Menu * subtypeMenu_ = nullptr;
    Info * description_ = nullptr;
    VariableInfo * status_ = nullptr;
    ColorDecorator * statusColor_ = nullptr;
    GameSnapshot snapshot_{};

    void layout();
    void updateTexts();
    void focusBottomButton(std::size_t index);
    void focusNextBottomButton();
    void focusPrevBottomButton();
};

/** @brief Main gameplay screen with world map, stats, and action buttons. */
class GameScreen final : public Screen {
public:
    GameScreen(Config & cfg, Window & win);

    request::UIRequest handleInput(int key) override;
    void resize() override;
    /** @brief Update map colors, stats, news ticker, and selected-country info. */
    void updateSnapshot(const GameSnapshot & snapshot);
private:
    int indexOfSelectedCountry = -1; // -1 == world
    bool navigatingCountries_ = true;
    bool countryMapsLoaded_ = false;
    Resolutions loadedMapResolution_ = Resolutions::Low;
    GameSnapshot snapshot_{};
    VariableInfo * pointsInfo_ = nullptr;
    VariableInfo * infectedInfo_ = nullptr;
    VariableInfo * deadInfo_ = nullptr;
    VariableInfo * cureInfo_ = nullptr;
    VariableInfo * dayInfo_ = nullptr;
    VariableInfo * selectedCountryInfo_ = nullptr;
    Ticker * newsTicker_ = nullptr;
    std::size_t displayedNewsCount_ = 0;
    std::vector<DetalizedImage *> countryImages_;
    std::vector<Rect> countryBounds_;

    void layout();
    void loadCountryMaps();
    void focusCountry(std::size_t countryIndex);
    void focusNearestCountry(int key);
    void focusActionButton(std::size_t buttonIndex);
    void focusNextActionButton();
    void focusPrevActionButton();
    void toggleNavigationMode();
    void showWorldOverview();
    void updateNewsTicker();
    void updateCountryStyles();
    void updatePopulationInfo();
    void updateSelectedCountryInfo();
};

/** @brief Victory/defeat summary screen. */
class EndScreen final : public Screen {
public:
    EndScreen(Config & cfg, Window & win);

    request::UIRequest handleInput(int key) override;
    void resize() override;
    /** @brief Update displayed game result from the final snapshot. */
    void updateSnapshot(const GameSnapshot & snapshot);
private:
    Info * resultInfo_ = nullptr;
    GameSnapshot snapshot_{};

    void layout();
    void updateText();
};

/**
 * @brief Base screen for game information pages with a shared tab bar.
 */
class InfoNavigationScreen : public Screen {
public:
    InfoNavigationScreen(Config & cfg, Window & win);

    request::UIRequest handleInput(int key) override;
    void resize() override;
protected:
    static constexpr std::size_t bodyWidgetStart = 4;

    /** @brief Layout the shared navigation tab bar. */
    void layoutNavigation();
    /** @return Rectangle available for page-specific body content. */
    Rect bodyRect() const;
    /** @brief Hook for derived screens to layout their own body widgets. */
    virtual void layoutBody() {}
    /** @brief Hook called after input handling. */
    virtual void afterHandledInput() {}
};

/** @brief Pathogen/player information tab. */
class PathogenInfoScreen final : public InfoNavigationScreen {
public:
    PathogenInfoScreen(Config & cfg, Window & win);

    void resize() override;
    /** @brief Update information text from a game snapshot. */
    void updateSnapshot(const GameSnapshot & snapshot);
private:
    Info * pathogenInfo_ = nullptr;
    GameSnapshot snapshot_{};

    void layout();
    void updateInfo();
};

/** @brief Cure progress and humanity response information tab. */
class CureInfoScreen final : public InfoNavigationScreen {
public:
    CureInfoScreen(Config & cfg, Window & win);

    void resize() override;
    /** @brief Update cure text from a game snapshot. */
    void updateSnapshot(const GameSnapshot & snapshot);
private:
    Info * cureInfo_ = nullptr;
    GameSnapshot snapshot_{};

    void layout();
    void updateInfo();
};

/** @brief Country list and selected-country details tab. */
class CountryScreen final : public InfoNavigationScreen {
public:
    CountryScreen(Config & cfg, Window & win);

    void resize() override;
    /** @brief Update country list details from a game snapshot. */
    void updateSnapshot(const GameSnapshot & snapshot);
private:
    Menu * countryMenu_ = nullptr;
    Info * countryInfo_ = nullptr;
    GameSnapshot snapshot_{};

    void layout();
    void updateSelectedCountryInfo();
    void afterHandledInput() override;
};

/** @brief Scrollable news history tab. */
class NewsScreen final : public InfoNavigationScreen {
public:
    NewsScreen(Config & cfg, Window & win);

    request::UIRequest handleInput(int key) override;
    void resize() override;
    /** @brief Update news lines from a game snapshot. */
    void updateSnapshot(const GameSnapshot & snapshot);
private:
    Info * newsInfo_ = nullptr;
    GameSnapshot snapshot_{};
    std::vector<std::string> newsLines_{};
    std::size_t scrollOffset_ = 0;

    void layout();
    void layoutBody() override;
    void updateNews();
    void renderNews();
    void scrollNews(int delta);
};

/** @brief Upgrade category screen with tabs, list, and description panel. */
class UpgradeScreen final : public Screen {
public:
    /**
     * @brief Construct an upgrade screen for one category.
     * @param category Initially selected upgrade category.
     */
    UpgradeScreen(Config & cfg, Window & win, UpgradeCategory category);

    request::UIRequest handleInput(int key) override;
    void resize() override;
    /** @brief Update upgrade availability and description from a snapshot. */
    void updateSnapshot(const GameSnapshot & snapshot);
private:
    static constexpr std::size_t tabCount_ = 3;
    static constexpr std::size_t listIndex_ = tabCount_;
    static constexpr std::size_t descriptionIndex_ = tabCount_ + 1;

    UpgradeCategory category_;
    UpgradeList * upgradeList_ = nullptr;
    Info * description_ = nullptr;
    GameSnapshot snapshot_{};

    void layout();
    void updateDescription();
    void focusTab(std::size_t tabIndex);
    void focusNextTab();
    void focusPrevTab();
    request::UIRequest purchaseSelectedUpgrade() const;
};

}
