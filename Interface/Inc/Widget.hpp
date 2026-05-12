#pragma once

#include "UIRequest.hpp"
#include "Upgrade.hpp"
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
    Rect rect() const { return rect_; }

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
    void changeText(std::string newText);
};

class VariableInfo final : public Widget {
private:
    std::string line_;
public:
    VariableInfo(Window & win, std::string text);
    void draw() override;
    void changeLine(std::string newLine);
};

class Ticker final : public Widget {
private:
    std::size_t speed_;
    std::size_t timer_ = 0;
    std::size_t visibleLength_ = 0;
    std::string curLine_;
    std::vector<std::string> linesQueue_;

    void loadNextLine();
public:
    Ticker(Window & win, std::size_t speed);
    void draw() override;
    void addLine(std::string newLine);
};

class Info final : public Widget {
private:
    std::vector<std::string> lines_;
public:
    Info(Window & win, std::string text);
    void draw() override;
    void changeText(std::string newText);
};

class Dialog final : public Widget {
private:
    std::vector<std::string> lines_;
    std::vector<std::unique_ptr<Button>> buttons_;
    std::size_t selectedIndex_ = 0;

    void layoutButtons();
    std::size_t selectableCount() const;
    void select(std::size_t index);
public:
    Dialog(Window & win, std::string text);
    void addButton(std::string text, std::function<request::UIRequest()> cb);
    void setRect(Rect rect) override;
    void setFocus(bool value) override;
    void draw() override;
    InputResult handleInput(int key) override;
    bool focusable() const override { return !buttons_.empty(); }
};

class TextInput final : public Widget {
    std::string text_{};
    std::size_t cursor_ = 0;
public:
    TextInput(Window & win);
    void draw() override;
    InputResult handleInput(int key) override;
    bool focusable() const override {return true;}
    std::string getText();
};

struct SymbolOnScreen {
    int y, x;
    std::string symbol;
};

class DetalizedImage final : public Widget {
    std::vector<SymbolOnScreen> symbols;
public:
    DetalizedImage(Window & win);
    void draw() override;
    bool focusable() const override { return true; }
    void clearSymbols();
    void addSymbol(SymbolOnScreen newSymbol);
    void addSymbols(std::vector<SymbolOnScreen> newSymbols);
};

//Decorators
class WidgetDecorator : public Widget {
protected:
    std::unique_ptr<Widget> inner_;
public:
    WidgetDecorator(Window & win, std::unique_ptr<Widget> inner);

    void setFocus(bool value) override;
    InputResult handleInput(int key) override;
    bool focusable() const override;
};

class FrameDecorator final : public WidgetDecorator {
public:
    FrameDecorator(Window & win, std::unique_ptr<Widget> inner);

    void setRect(Rect rect) override;
    void draw() override;
};

class LabelDecorator final : public WidgetDecorator {
    std::string label_;
public:
    LabelDecorator(Window & win, std::unique_ptr<Widget> inner, std::string label);

    void setRect(Rect rect) override;
    void draw() override;
};

class ColorDecorator final : public WidgetDecorator {
    int colorPair_;
public:
    ColorDecorator(Window & win, std::unique_ptr<Widget> inner, int colorPair);

    void setRect(Rect rect) override;
    void draw() override;

    void setColorPair(int newColorPair);
};

class Menu final : public Widget {
private:
    std::vector<std::unique_ptr<Button>> buttons_;
    std::size_t selectedIndex_ = 0;
    std::size_t firstVisibleIndex_ = 0;

    void layoutButtons();
    std::size_t selectableCount() const;
    void select(std::size_t index);
public:
    explicit Menu(Window & win);

    void addButton(std::string text, std::function<request::UIRequest()> cb);
    void changeButtonText(std::size_t index, std::string text);
    void setRect(Rect rect) override;
    void setFocus(bool value) override;
    void draw() override;
    InputResult handleInput(int key) override;
    bool focusable() const override { return !buttons_.empty(); }
    std::size_t selectedIndex() const { return selectedIndex_; }
};

struct UpgradeListItem {
    UpgradeDefinition upgrade;
    bool available = false;
    bool purchased = false;
};

class UpgradeList final : public Widget {
private:
    std::vector<UpgradeListItem> items_;
    std::size_t selectedIndex_ = 0;
    std::size_t firstVisibleIndex_ = 0;

    std::size_t selectableCount() const;
    void select(std::size_t index);
public:
    explicit UpgradeList(Window & win);

    void setItems(std::vector<UpgradeListItem> items);
    void setRect(Rect rect) override;
    void setFocus(bool value) override;
    void draw() override;
    InputResult handleInput(int key) override;
    bool focusable() const override { return !items_.empty(); }
    std::size_t selectedIndex() const { return selectedIndex_; }
    const UpgradeListItem * selectedItem() const;
};

class TabBar final : public Widget {
private:
    std::vector<std::unique_ptr<Button>> buttons_;
    std::size_t selectedIndex_ = 0;

    void layoutButtons();
    std::size_t selectableCount() const;
    void select(std::size_t index);
public:
    explicit TabBar(Window & win);

    void addButton(std::string text, std::function<request::UIRequest()> cb);
    void setRect(Rect rect) override;
    void setFocus(bool value) override;
    void draw() override;
    InputResult handleInput(int key) override;
    bool focusable() const override { return !buttons_.empty(); }
};

}
