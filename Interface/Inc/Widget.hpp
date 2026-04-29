#pragma once

#include "UIRequest.hpp"
#include "Window.hpp"
#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace plague::ui {

struct Rect {
    int y;
    int x;
    int height;
    int width;
};

struct InputResult {
    bool handled = false;
    request::UIRequest request = request::None{};
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
    virtual InputResult handleInput(int key) {
        (void)key;
        return {};
    }

    virtual bool focusable() const { return false; }

    virtual void setRect(Rect rect) { rect_ = rect; }

    virtual void setFocus(bool value) { focused_ = value; }
    bool focused() const { return focused_; }
};

class Button final : public Widget {
private:
    std::string text_;
    std::function<request::UIRequest()> onClick_;
public:
    Button(Window & win, std::string text, std::function<request::UIRequest()> cb);
    void draw() override;
    InputResult handleInput(int key) override;
    bool focusable() const override { return true; }
};

class Info final : public Widget {
private:
    std::vector<std::string> lines_;
public:
    Info(Window & win, std::string text);
    void draw() override;
};

class Menu final : public Widget {
private:
    std::vector<std::unique_ptr<Button>> buttons_;
    std::size_t selectedIndex_ = 0;

    void layoutButtons();
    std::size_t selectableCount() const;
    void select(std::size_t index);
public:
    explicit Menu(Window & win);

    void addButton(std::string text, std::function<request::UIRequest()> cb);
    void setRect(Rect rect) override;
    void setFocus(bool value) override;
    void draw() override;
    InputResult handleInput(int key) override;
    bool focusable() const override { return !buttons_.empty(); }
};

}
