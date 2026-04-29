#pragma once

#include "UIRequest.hpp"
#include "Window.hpp"
#include <functional>
#include <string>
#include <vector>

namespace plague::ui {

struct Rect {
    int y;
    int x;
    int height;
    int width;
};

class Widget {
protected:
    Window & win_;
    Rect rect_{};
    bool focused_ = false;
public:
    Widget(Window & win);
    virtual ~Widget() = default;

    virtual void draw() = 0;
    virtual request::UIRequest handleInput(int key) {
        (void)key;
        return request::None{};
    }

    virtual bool focusable() const { return false; }

    void setRect(Rect rect) { rect_ = rect; }

    void setFocus(bool value) { focused_ = value; }
    bool focused() const { return focused_; }
};

class Button final : public Widget {
private:
    std::string text_;
    std::function<request::UIRequest()> onClick_;
public:
    Button(Window & win, std::string text, std::function<request::UIRequest()> cb);
    void draw() override;
    request::UIRequest handleInput(int key) override;
    bool focusable() const override { return true; }
};

class Info final : public Widget {
private:
    std::vector<std::string> lines_;
public:
    Info(Window & win, std::string text);
    void draw() override;
};

}
