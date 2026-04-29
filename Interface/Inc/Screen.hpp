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

    virtual void draw() = 0;
    virtual request::UIRequest handleInput(int key) = 0;
    virtual void resize();
    int getKey();
    void add(std::unique_ptr<Widget> newWidget);
    Screen(Config & cfg, Window & win);
protected:
    std::vector<std::unique_ptr<Widget>> widgets{};
    Config & cfg_;
    Window & win_;

    void focusWidget(std::size_t index);
    Widget * focusedWidget();
    std::size_t focusedIndex_ = 0;
};

class MainMenuScreen final: public Screen {
public:
    MainMenuScreen(Config & cfg, Window & win);

    void draw() override;
    request::UIRequest handleInput(int key) override;
    void resize() override;
private:
    void layout();
};

}
