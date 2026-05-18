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

class Screen {
public:
    virtual ~Screen() = default;

    void draw();
    virtual request::UIRequest handleInput(int key) = 0;
    virtual void resize();
    int getKey();
    Screen(Config & cfg, Window & win);
protected:
    std::vector<std::unique_ptr<Widget>> widgets{};
    Config & cfg_;
    Window & win_;

    void focusFirst();
    void focusWidget(std::size_t index);
    void focusNext();
    void focusPrev();
    Widget * focusedWidget();
    InputResult handleFocusedInput(int key);
    std::size_t focusedIndex_ = 0;
};

class MainMenuScreen final: public Screen {
public:
    MainMenuScreen(Config & cfg, Window & win);

    request::UIRequest handleInput(int key) override;
    void resize() override;
private:
    void layout();
};

class SmallTermScreen final: public Screen {
public:
    SmallTermScreen(Config & cfg, Window & win);

    request::UIRequest handleInput(int key) override;
    void resize() override;
private:
    void layout();
};

class ConnectToServerScreen final : public Screen {
public:
    ConnectToServerScreen(Config & cfg, Window & win);

    request::UIRequest handleInput(int key) override;
    void resize() override;
private:
	    void layout();
	};

class RoomBrowserScreen final : public Screen {
public:
    RoomBrowserScreen(Config & cfg, Window & win);

    request::UIRequest handleInput(int key) override;
    void resize() override;
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

    void layout();
    void updateRooms();
    void updateStatus();
    request::UIRequest joinSelectedRoom() const;
    request::UIRequest createRoom() const;
};

class ChoosingSideScreen final : public Screen {
public:
    ChoosingSideScreen(Config & cfg, Window & win);

    request::UIRequest handleInput(int key) override;
    void resize() override;
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

class GameScreen final : public Screen {
public:
    GameScreen(Config & cfg, Window & win);

    request::UIRequest handleInput(int key) override;
    void resize() override;
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
    void updatePopulationInfo();
    void updateSelectedCountryInfo();
};

class InfoNavigationScreen : public Screen {
public:
    InfoNavigationScreen(Config & cfg, Window & win);

    request::UIRequest handleInput(int key) override;
    void resize() override;
protected:
    static constexpr std::size_t bodyWidgetStart = 4;

    void layoutNavigation();
    Rect bodyRect() const;
    virtual void layoutBody() {}
    virtual void afterHandledInput() {}
};

class PathogenInfoScreen final : public InfoNavigationScreen {
public:
    PathogenInfoScreen(Config & cfg, Window & win);

    void resize() override;
private:
    void layout();
};

class CureInfoScreen final : public InfoNavigationScreen {
public:
    CureInfoScreen(Config & cfg, Window & win);

    void resize() override;
private:
    void layout();
};

class CountryScreen final : public InfoNavigationScreen {
public:
    CountryScreen(Config & cfg, Window & win);

    void resize() override;
private:
    Menu * countryMenu_ = nullptr;
    Info * countryInfo_ = nullptr;

    void layout();
    void updateSelectedCountryInfo();
    void afterHandledInput() override;
};

class NewsScreen final : public InfoNavigationScreen {
public:
    NewsScreen(Config & cfg, Window & win);

    void resize() override;
private:
    void layout();
};

class UpgradeScreen final : public Screen {
public:
    UpgradeScreen(Config & cfg, Window & win, UpgradeCategory category);

    request::UIRequest handleInput(int key) override;
    void resize() override;
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
};

}
