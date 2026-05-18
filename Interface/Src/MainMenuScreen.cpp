#include "ScreenShared.hpp"

namespace plague::ui {

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
    const int logoHeight = 9;
    const int buttonY = cfg_.resolution == Resolutions::Low ? 28 : (cfg_.resolution == Resolutions::Medium ? 30 : 32);
    const int buttonWidth = 20;

    if (widgets.size() < 2) return;

    widgets[0]->setRect({padding, padding, logoHeight, contentWidth});
    widgets[1]->setRect({buttonY, padding, 3, buttonWidth});
}

void MainMenuScreen::resize() {
    layout();
}

request::UIRequest MainMenuScreen::handleInput(int key) {
    const InputResult result = handleFocusedInput(key);
    if (!std::holds_alternative<request::None>(result.request)) {
        return result.request;
    }

    if (result.handled) {
        return request::None{};
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

}  // namespace plague::ui
