#pragma once

#include "UIBase.hpp"
#include "UIRequest.hpp"

#include <cstddef>
#include <string_view>

namespace plague::ui {

enum MainMenuItemId : unsigned {
    ConnectToServer,
    Settings,
    Exit,
};

struct MainMenuItem {
    MainMenuItemId id;
    int left;
    int right;
    int up;
    int down;
    std::string_view text;
    plague::request::UIRequest action;
};

struct MainMenuState {
    size_t selectedItem = ConnectToServer;
};

class MainMenuScreen final : public Screen {
public:
    MainMenuScreen() = default;
    ~MainMenuScreen() override = default;

    void draw() const override;
    plague::request::UIRequest handleInput(int key) override;
private:
    MainMenuState state_{};

    void moveUp();
    void moveDown();
    plague::request::UIRequest activateCurrentItem() const;

    static constexpr MainMenuItem kMenuItems[] = {
        {MainMenuItemId::ConnectToServer, -1, -1, -1,  1, "Connect to server", request::MainMenu::ConnectToServer},
        {MainMenuItemId::Settings,        -1, -1,  0,  2, "Settings", request::MainMenu::OpenSettings},
        {MainMenuItemId::Exit,            -1, -1,  1, -1, "Exit", request::MainMenu::Exit}
    };
};

}