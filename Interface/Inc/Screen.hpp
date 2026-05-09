#pragma once

#include "Settings.hpp"
#include "UIRequest.hpp"
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
    void add(std::unique_ptr<Widget> newWidget);
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

class GameScreen final : public Screen {
public:
    GameScreen(Config & cfg, Window & win);

    request::UIRequest handleInput(int key) override;
    void resize() override;
private:
    int indexOfSelectedCountry = -1; // -1 == world
    bool navigatingCountries_ = true;
    bool countryMapsLoaded_ = false;
    Resolutions loadedMapResolution_ = Resolutions::Low;
    VariableInfo * selectedCountryInfo_ = nullptr;
    std::vector<DetalizedImage *> countryImages_;

    void layout();
    void loadCountryMaps();
    void focusCountry(std::size_t countryIndex);
    void focusNextCountry();
    void focusPrevCountry();
    void focusActionButton(std::size_t buttonIndex);
    void focusNextActionButton();
    void focusPrevActionButton();
    void toggleNavigationMode();
    bool focusedOnCountry() const;
    void updateSelectedCountryInfo();
};

class InfoScreen final : public Screen {
public:
    InfoScreen(Config & cfg, Window & win);

    request::UIRequest handleInput(int key) override;
    void resize() override;
private:
    void layout();
};

class TransmissionScreen final : public Screen {
public:
    TransmissionScreen(Config & cfg, Window & win);

    request::UIRequest handleInput(int key) override;
    void resize() override;
private:
    void layout();
};

class ClinicScreen final : public Screen {
public:
    ClinicScreen(Config & cfg, Window & win);

    request::UIRequest handleInput(int key) override;
    void resize() override;
private:
    void layout();
};

class AbilitiesScreen final : public Screen {
public:
    AbilitiesScreen(Config & cfg, Window & win);

    request::UIRequest handleInput(int key) override;
    void resize() override;
private:
    void layout();
};

class WorldScreen final : public Screen {
public:
    WorldScreen(Config & cfg, Window & win);

    request::UIRequest handleInput(int key) override;
    void resize() override;
private:
    void layout();
};

class CureScreen final : public Screen {
public:
    CureScreen(Config & cfg, Window & win);

    request::UIRequest handleInput(int key) override;
    void resize() override;
private:
    void layout();
};

class NewsScreen final : public Screen {
public:
    NewsScreen(Config & cfg, Window & win);

    request::UIRequest handleInput(int key) override;
    void resize() override;
private:
    void layout();
};

}
