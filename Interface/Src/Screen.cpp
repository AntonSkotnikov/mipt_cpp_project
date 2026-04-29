#include "Screen.hpp"
#include "Settings.hpp"
#include "UIRequest.hpp"
#include "Widget.hpp"
#include "Window.hpp"
#include <cstddef>
#include <memory>

namespace plague::ui {

Screen::Screen(Config & cfg, Window & mainWin) : cfg_(cfg), win_(mainWin) {};

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

    auto settingsButton = std::make_unique<Button>(win_, "Connect to server", []() -> request::UIRequest {
        return request::MainMenu::OpenSettings;
    });

    auto quitButton = std::make_unique<Button>(win_, "Quit", []() -> request::UIRequest {
        return request::MainMenu::Exit;
    });

    // setRect
    switch (cfg_.resolution) {
        case Resolutions::Low:

        case Resolutions::Medium:
        case Resolutions::High:
            break;
    }

    widgets.push_back(std::move(logoWidget));
    widgets.push_back(std::move(connectButton));
    widgets.push_back(std::move(settingsButton));
    widgets.push_back(std::move(quitButton));
} 

void MainMenuScreen::draw() {
    size_t numOfWidgets = widgets.size();
    for (size_t i = 0; i < numOfWidgets; i++) {
        widgets[i]->draw();
    }
}

void MainMenuScreen::handleInput(int key) {
    size_t numOfWidgets = widgets.size();
    for (size_t i = 0; i < numOfWidgets; i++) {
        if (widgets[i]->focused() && widgets[i]->handleInput(key)) return;
    }

    // TODO navigation
}

}