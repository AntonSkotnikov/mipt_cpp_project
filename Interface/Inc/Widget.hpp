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

/**
 * @brief Rectangle in window-local coordinates.
 */
struct Rect {
    /** Top row. */
    int y;
    /** Left column. */
    int x;
    /** Height in rows. */
    int height;
    /** Width in columns. */
    int width;
};

/**
 * @brief Result of widget input handling.
 */
struct InputResult {
    /** True when the widget consumed the key even if it did not emit a request. */
    bool handled = false;
    /** Request emitted by the widget. */
    request::UIRequest request = request::None{};
};

/**
 * @brief Base class for every drawable and optionally focusable UI widget.
 */
class Widget {
protected:
    Window & win_;
    Rect rect_{};
    bool focused_ = false;
public:
    /** @brief Construct a widget bound to a window. */
    Widget(Window & win);
    virtual ~Widget() = default;

    /** @brief Draw the widget into its assigned rectangle. */
    virtual void draw() = 0;
    /**
     * @brief Handle one input key.
     * @param key ncurses key code.
     * @return Handling status and optional UI request.
     */
    virtual InputResult handleInput(int key) {
        (void)key;
        return {};
    }

    /** @return true if the widget can receive focus. */
    virtual bool focusable() const { return false; }

    /** @brief Assign widget bounds. */
    virtual void setRect(Rect rect) { rect_ = rect; }
    /** @return Current widget bounds. */
    Rect rect() const { return rect_; }

    /** @brief Set focus state. */
    virtual void setFocus(bool value) { focused_ = value; }
    /** @return true when the widget is currently focused. */
    bool focused() const { return focused_; }
};

/** @brief Single-line focusable button that calls a callback on Enter. */
class Button final : public Widget {
private:
    std::string text_;
    std::function<request::UIRequest()> onClick_;
public:
    Button(Window & win, std::string text, std::function<request::UIRequest()> cb);
    void draw() override;
    InputResult handleInput(int key) override;
    bool focusable() const override { return true; }
    /** @brief Replace button label text. */
    void changeText(std::string newText);
};

/** @brief One-line read-only text widget used for changing stats. */
class VariableInfo final : public Widget {
private:
    std::string line_;
public:
    VariableInfo(Window & win, std::string text);
    void draw() override;
    /** @brief Replace the displayed line. */
    void changeLine(std::string newLine);
};

/** @brief Typewriter-style scrolling news ticker. */
class Ticker final : public Widget {
private:
    std::size_t speed_;
    std::size_t timer_ = 0;
    std::size_t visibleLength_ = 0;
    std::string curLine_;
    std::vector<std::string> linesQueue_;

    void loadNextLine();
public:
    /**
     * @brief Construct a ticker.
     * @param speed Number of frames between visible-character updates.
     */
    Ticker(Window & win, std::size_t speed);
    void draw() override;
    /** @brief Queue a new line for ticker playback. */
    void addLine(std::string newLine);
};

/** @brief Multi-line read-only text widget with word wrapping. */
class Info final : public Widget {
private:
    std::vector<std::string> lines_;
public:
    Info(Window & win, std::string text);
    void draw() override;
    /** @brief Replace all displayed text. */
    void changeText(std::string newText);
};

/** @brief Modal-like text block with vertically arranged buttons. */
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
    /** @brief Add a focusable action button to the dialog. */
    void addButton(std::string text, std::function<request::UIRequest()> cb);
    void setRect(Rect rect) override;
    void setFocus(bool value) override;
    void draw() override;
    InputResult handleInput(int key) override;
    bool focusable() const override { return !buttons_.empty(); }
};

/** @brief Single-line editable text input. */
class TextInput final : public Widget {
    std::string text_{};
    std::size_t cursor_ = 0;
public:
    TextInput(Window & win);
    void draw() override;
    InputResult handleInput(int key) override;
    bool focusable() const override {return true;}
    /** @return Current input text. */
    std::string getText();
};

/** @brief One drawable symbol loaded from a country map asset. */
struct SymbolOnScreen {
    /** Symbol row relative to the image widget. */
    int y, x;
    /** UTF-8 symbol text. */
    std::string symbol;
};

/**
 * @brief Drawable country/map image made of positioned text symbols.
 *
 * The widget supports focus borders, event borders, and a base color pair.
 */
class DetalizedImage final : public Widget {
    std::vector<SymbolOnScreen> symbols;
    std::vector<bool> borderSymbols_;
    bool eventHighlighted_ = false;
    int colorPair_ = 0;

    void updateBorderSymbols();
public:
    DetalizedImage(Window & win);
    void draw() override;
    bool focusable() const override { return true; }
    /** @brief Remove all symbols from the image. */
    void clearSymbols();
    /** @brief Add a single symbol and recompute border markers. */
    void addSymbol(SymbolOnScreen newSymbol);
    /** @brief Add many symbols and recompute border markers. */
    void addSymbols(std::vector<SymbolOnScreen> newSymbols);
    /** @brief Enable or disable event border highlighting. */
    void setEventHighlight(bool value);
    /** @brief Set the base color pair used for non-border symbols. */
    void setColorPair(int newColorPair);
};

/** @brief Base class for widgets that wrap another widget. */
class WidgetDecorator : public Widget {
protected:
    std::unique_ptr<Widget> inner_;
public:
    WidgetDecorator(Window & win, std::unique_ptr<Widget> inner);

    void setFocus(bool value) override;
    InputResult handleInput(int key) override;
    bool focusable() const override;
};

/** @brief Decorator that draws an ncurses-style frame around another widget. */
class FrameDecorator final : public WidgetDecorator {
public:
    FrameDecorator(Window & win, std::unique_ptr<Widget> inner);

    void setRect(Rect rect) override;
    void draw() override;
};

/** @brief Decorator that draws a label above another widget. */
class LabelDecorator final : public WidgetDecorator {
    std::string label_;
public:
    LabelDecorator(Window & win, std::unique_ptr<Widget> inner, std::string label);

    void setRect(Rect rect) override;
    void draw() override;
};

/** @brief Decorator that draws another widget under an ncurses color pair. */
class ColorDecorator final : public WidgetDecorator {
    int colorPair_;
public:
    ColorDecorator(Window & win, std::unique_ptr<Widget> inner, int colorPair);

    void setRect(Rect rect) override;
    void draw() override;

    /** @brief Change the applied color pair. */
    void setColorPair(int newColorPair);
};

/** @brief Vertical list of buttons with scrolling and selection. */
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

    /** @brief Add a button to the menu. */
    void addButton(std::string text, std::function<request::UIRequest()> cb);
    /** @brief Replace the text of an existing button. */
    void changeButtonText(std::size_t index, std::string text);
    void setRect(Rect rect) override;
    void setFocus(bool value) override;
    void draw() override;
    InputResult handleInput(int key) override;
    bool focusable() const override { return !buttons_.empty(); }
    /** @return Index of the currently selected visible/logical button. */
	    std::size_t selectedIndex() const { return selectedIndex_; }
	};

/** @brief Room row shown in the room browser. */
struct RoomListItem {
    std::string name;
    bool privateRoom = false;
    std::size_t players = 0;
    std::size_t capacity = 2;
};

/** @brief Focusable list of rooms in the room browser. */
class RoomList final : public Widget {
private:
    std::vector<RoomListItem> items_;
    std::size_t selectedIndex_ = 0;
    std::size_t firstVisibleIndex_ = 0;

    std::size_t selectableCount() const;
    void select(std::size_t index);
public:
    explicit RoomList(Window & win);

    /** @brief Replace all displayed rooms. */
    void setItems(std::vector<RoomListItem> items);
    void setRect(Rect rect) override;
    void setFocus(bool value) override;
    void draw() override;
    InputResult handleInput(int key) override;
    bool focusable() const override { return !items_.empty(); }
    /** @return Currently selected room index. */
    std::size_t selectedIndex() const { return selectedIndex_; }
    /** @return Selected item, or nullptr when the list is empty. */
    const RoomListItem * selectedItem() const;
};

/** @brief Upgrade row shown in upgrade lists. */
struct UpgradeListItem {
    UpgradeDefinition upgrade;
    bool available = false;
    bool purchased = false;
};

/** @brief Focusable list of upgrades with availability/purchase state. */
class UpgradeList final : public Widget {
private:
    std::vector<UpgradeListItem> items_;
    std::size_t selectedIndex_ = 0;
    std::size_t firstVisibleIndex_ = 0;

    std::size_t selectableCount() const;
    void select(std::size_t index);
public:
    explicit UpgradeList(Window & win);

    /** @brief Replace all displayed upgrades. */
    void setItems(std::vector<UpgradeListItem> items);
    void setRect(Rect rect) override;
    void setFocus(bool value) override;
    void draw() override;
    InputResult handleInput(int key) override;
    bool focusable() const override { return !items_.empty(); }
    /** @return Currently selected upgrade index. */
    std::size_t selectedIndex() const { return selectedIndex_; }
    /** @return Selected item, or nullptr when the list is empty. */
    const UpgradeListItem * selectedItem() const;
};

/** @brief Horizontal group of tab buttons. */
class TabBar final : public Widget {
private:
    std::vector<std::unique_ptr<Button>> buttons_;
    std::size_t selectedIndex_ = 0;

    void layoutButtons();
    std::size_t selectableCount() const;
    void select(std::size_t index);
public:
    explicit TabBar(Window & win);

    /** @brief Add a tab button. */
    void addButton(std::string text, std::function<request::UIRequest()> cb);
    void setRect(Rect rect) override;
    void setFocus(bool value) override;
    void draw() override;
    InputResult handleInput(int key) override;
    bool focusable() const override { return !buttons_.empty(); }
};

}
