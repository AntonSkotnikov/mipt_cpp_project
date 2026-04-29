#include "Screen.hpp"
#include "Settings.hpp"
#include "UIRequest.hpp"
#include "Widget.hpp"
#include "Window.hpp"
#include <algorithm>
#include <cstddef>
#include <memory>


namespace plague::ui {

Screen::Screen(Config & cfg, Window & mainWin) : cfg_(cfg), win_(mainWin) {};

int Screen::getKey() {
    return win_.getKey();
}

void Screen::focusWidget(std::size_t index) {
    if (index >= widgets.size() || !widgets[index]->focusable()) return;

    if (focusedIndex_ < widgets.size()) {
        widgets[focusedIndex_]->setFocus(false);
    }

    focusedIndex_ = index;
    widgets[focusedIndex_]->setFocus(true);
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
            
    auto connectButton = std::make_unique<Button>(win_, "Connect to server", []() -> request::UIRequest {
        return request::MainMenu::ConnectToServer;
    });

    auto settingsButton = std::make_unique<Button>(win_, "Settings", []() -> request::UIRequest {
        return request::MainMenu::OpenSettings;
    });

    auto quitButton = std::make_unique<Button>(win_, "Quit", []() -> request::UIRequest {
        return request::MainMenu::Exit;
    });

    const int contentWidth = std::max(1, win_.width() - 4);
    const int logoHeight = std::min(8, std::max(1, win_.height() - 8));
    const int buttonY = std::min(logoHeight + 4, std::max(2, win_.height() - 4));
    const int buttonWidth = std::min(20, contentWidth);

    logoWidget->setRect({2, 2, logoHeight, contentWidth});
    connectButton->setRect({buttonY, 2, 1, buttonWidth});
    settingsButton->setRect({buttonY + 1, 2, 1, buttonWidth});
    quitButton->setRect({buttonY + 2, 2, 1, buttonWidth});

    widgets.push_back(std::move(logoWidget));
    widgets.push_back(std::move(connectButton));
    widgets.push_back(std::move(settingsButton));
    widgets.push_back(std::move(quitButton));

    for (auto & widget : widgets) {
        if (widget->focusable()) {
            widget->setFocus(true);
            break;
        }
    }
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
    size_t numOfWidgets = widgets.size();
    for (size_t i = 0; i < numOfWidgets; i++) {
        if (widgets[i]->focused()) {
            request::UIRequest req =  widgets[i]->handleInput(key);
            if (!std::holds_alternative<request::None>(req)) return req;
        }
    }

    switch (key) {
        case KEY_UP:
            focusWidget(focusedIndex_ == 1 ? 3 : focusedIndex_ - 1);
            return request::None{};

        case KEY_DOWN:
            focusWidget(focusedIndex_ == 3 ? 1 : focusedIndex_ + 1);
            return request::None{};

        default:
            if (Widget * widget = focusedWidget()) {
                return widget->handleInput(key);
            }

            return request::None{};
    }
    // TODO navigation

    return request::None{};
}

}
