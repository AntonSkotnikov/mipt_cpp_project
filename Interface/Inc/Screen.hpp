#pragma once

#include "Settings.hpp"
#include "UIRequest.hpp"
#include "Window.hpp"
#include "Widget.hpp"
#include <memory>
#include <vector>

namespace plague::ui {

class Screen {
public:
    std::vector<std::unique_ptr<Widget>> widgets{};
    Config & cfg_;
    Window & win_;
    virtual ~Screen() = default;

    virtual void draw() = 0;
    virtual request::UIRequest handleInput(int key) = 0;
    int getKey();
    void add(std::unique_ptr<Widget> newWidget);
    Screen(Config & cfg, Window & win);
};

class MainMenuScreen final: public Screen {
public:
    MainMenuScreen(Config & cfg, Window & win);

    void draw() override;
    request::UIRequest handleInput(int key) override;
};

}
