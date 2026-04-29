#include "Screen.hpp"
#include "Settings.hpp"
#include "UIRequest.hpp"
#include "Widget.hpp"
#include "Window.hpp"
#include <algorithm>
#include <cstddef>
#include <memory>
#include <variant>


namespace plague::ui {

Screen::Screen(Config & cfg, Window & mainWin) : cfg_(cfg), win_(mainWin) {};

int Screen::getKey() {
    return win_.getKey();
}

void Screen::resize() {}

void Screen::focusFirst() {
    for (std::size_t i = 0; i < widgets.size(); i++) {
        if (widgets[i]->focusable()) {
            focusWidget(i);
            return;
        }
    }
}

void Screen::focusWidget(std::size_t index) {
    if (index >= widgets.size() || !widgets[index]->focusable()) return;

    if (focusedIndex_ < widgets.size()) {
        widgets[focusedIndex_]->setFocus(false);
    }

    focusedIndex_ = index;
    widgets[focusedIndex_]->setFocus(true);
}

void Screen::focusNext() {
    if (widgets.empty()) return;

    for (std::size_t step = 1; step <= widgets.size(); step++) {
        const std::size_t index = (focusedIndex_ + step) % widgets.size();
        if (widgets[index]->focusable()) {
            focusWidget(index);
            return;
        }
    }
}

void Screen::focusPrev() {
    if (widgets.empty()) return;

    for (std::size_t step = 1; step <= widgets.size(); step++) {
        const std::size_t index = (focusedIndex_ + widgets.size() - step) % widgets.size();
        if (widgets[index]->focusable()) {
            focusWidget(index);
            return;
        }
    }
}

Widget * Screen::focusedWidget() {
    if (focusedIndex_ >= widgets.size()) return nullptr;
    return widgets[focusedIndex_].get();
}


MainMenuScreen::MainMenuScreen(Config & cfg, Window & win) : Screen(cfg, win) {
    auto logoWidget = std::make_unique<Info>(win_, R"(
                ______ _                          _____             
                | ___ \ |                        |_   _|            
                | |_/ / | __ _  __ _ _   _  ___    | | _ __   ___   
                |  __/| |/ _` |/ _` | | | |/ _ \   | || '_ \ / __|  
                | |   | | (_| | (_| | |_| |  __/  _| || | | | (__ _ 
                \_|   |_|\__,_|\__, |\__,_|\___|  \___/_| |_|\___(_)
                                __/ |                               
                               |___/                                )");
            
    auto menuWidget = std::make_unique<Menu>(win_);

    menuWidget->addButton("Connect to server", []() -> request::UIRequest {
        return request::MainMenu::ConnectToServer;
    });

    menuWidget->addButton("Settings", []() -> request::UIRequest {
        return request::MainMenu::OpenSettings;
    });

    menuWidget->addButton("Quit", []() -> request::UIRequest {
        return request::MainMenu::Exit;
    });

    widgets.push_back(std::move(logoWidget));
    widgets.push_back(std::move(menuWidget));

    layout();

    focusFirst();
} 

void MainMenuScreen::layout() {
    const int padding = win_.bordered() ? 2 : 1;
    const int contentWidth = std::max(1, win_.width() - padding * 2);
    const int logoHeight = std::min(8, std::max(1, win_.height() - padding - 6));
    const int buttonY = std::min(padding + logoHeight + 2, std::max(padding, win_.height() - padding - 3));
    const int buttonWidth = std::min(20, contentWidth);

    if (widgets.size() < 2) return;

    widgets[0]->setRect({padding, padding, logoHeight, contentWidth});
    widgets[1]->setRect({buttonY, padding, 3, buttonWidth});
}

void MainMenuScreen::resize() {
    layout();
}

void MainMenuScreen::draw() {
    win_.clear();

    size_t numOfWidgets = widgets.size();
    for (size_t i = 0; i < numOfWidgets; i++) {
        widgets[i]->draw();
    }

    win_.refresh();
}

request::UIRequest MainMenuScreen::handleInput(int key) {
    if (Widget * widget = focusedWidget()) {
        const InputResult result = widget->handleInput(key);
        if (!std::holds_alternative<request::None>(result.request)) {
            return result.request;
        }

        if (result.handled) {
            return request::None{};
        }
    }

    switch (key) {
        case KEY_LEFT:
        case KEY_BTAB:
            focusPrev();
            return request::None{};

        case KEY_RIGHT:
        case '\t':
            focusNext();
            return request::None{};
    }

    return request::None{};
}

SmallTermScreen::SmallTermScreen(Config & cfg, Window & win) : Screen(cfg, win) {
    widgets.push_back(std::make_unique<FrameDecorator>(
        win_,
        std::make_unique<Dialog>(win_, "Terminal is too small\nPlease resize it")
    ));
    layout();
}

void SmallTermScreen::layout() {
    if (widgets.empty()) return;

    if (win_.height() <= 2 || win_.width() <= 2) {
        widgets[0]->setRect({0, 0, win_.height(), win_.width()});
        return;
    }

    widgets[0]->setRect({1, 1, win_.height() - 2, win_.width() - 2});
}

void SmallTermScreen::resize() {
    layout();
}

void SmallTermScreen::draw() {
    win_.clear();

    for (auto & widget : widgets) {
        widget->draw();
    }

    win_.refresh();
}

request::UIRequest SmallTermScreen::handleInput(int key) {
    (void)key;
    return request::None{};
}

ConnectToServerScreen::ConnectToServerScreen(Config & cfg, Window & win) : Screen(cfg, win) {
    widgets.push_back(std::make_unique<LabelDecorator>(win_, std::make_unique<FrameDecorator>(win_, std::make_unique<TextInput>(win_)), "Address"));
    widgets.push_back(std::make_unique<LabelDecorator>(win_, std::make_unique<FrameDecorator>(win_, std::make_unique<TextInput>(win_)), "Port"));

    auto menuWidget = std::make_unique<Menu>(win_);
    menuWidget->addButton("Connect", []() -> request::UIRequest {
        return request::Connect::Connect;
    });
    menuWidget->addButton("Back", []() -> request::UIRequest {
        return request::Connect::Back;
    });
    widgets.push_back(std::move(menuWidget));

    layout();
    focusFirst();
}

void ConnectToServerScreen::layout() {
    if (widgets.size() < 3) return;

    const int padding = win_.bordered() ? 2 : 1;
    const int contentWidth = std::max(1, win_.width() - padding * 2);
    const int fieldWidth = std::min(46, std::max(1, contentWidth - 2));
    const int x = padding;

    widgets[0]->setRect({padding + 2, x + 1, 1, fieldWidth});
    widgets[1]->setRect({padding + 7, x + 1, 1, fieldWidth});
    widgets[2]->setRect({padding + 11, x, 2, std::min(20, fieldWidth)});
}

void ConnectToServerScreen::resize() {
    layout();
}

void ConnectToServerScreen::draw() {
    win_.clear();

    for (auto & widget : widgets) {
        widget->draw();
    }

    win_.refresh();
}

request::UIRequest ConnectToServerScreen::handleInput(int key) {
    if (Widget * widget = focusedWidget()) {
        const InputResult result = widget->handleInput(key);
        if (!std::holds_alternative<request::None>(result.request)) {
            return result.request;
        }

        if (result.handled) {
            return request::None{};
        }
    }

    switch (key) {
        case KEY_UP:
        case KEY_BTAB:
            focusPrev();
            return request::None{};

        case KEY_DOWN:
        case '\t':
            focusNext();
            return request::None{};

        case 27:
            return request::Connect::Back;
    }

    return request::None{};
}

}
