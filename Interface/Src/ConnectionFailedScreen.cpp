#include "ScreenShared.hpp"

namespace plague::ui {

ConnectionFailedScreen::ConnectionFailedScreen(Config & cfg, Window & win) : Screen(cfg, win) {
    auto dialog = std::make_unique<Dialog>(
        win_,
        "Could not connect to the server\nCheck address, port, and server availability"
    );

    dialog->addButton("Try again", [this]() -> request::UIRequest {
        return request::ConnectInfo{
            request::Connect::Connect,
            cfg_.lastConnectAddr,
            cfg_.lastConnectPort
        };
    });

    dialog->addButton("Back", []() -> request::UIRequest {
        return request::ConnectInfo{request::Connect::Back, "", ""};
    });

    widgets.push_back(std::make_unique<FrameDecorator>(win_, std::move(dialog)));

    layout();
    focusFirst();
}

void ConnectionFailedScreen::layout() {
    if (widgets.empty()) return;

    const int padding = win_.bordered() ? 2 : 1;
    const int contentWidth = std::max(1, win_.width() - padding * 2);
    const int contentHeight = std::max(1, win_.height() - padding * 2);
    const int dialogWidth = std::min(64, std::max(32, contentWidth / 2));
    const int dialogHeight = 9;
    const int y = padding + std::max(0, (contentHeight - dialogHeight) / 2);
    const int x = padding + std::max(0, (contentWidth - dialogWidth) / 2);

    widgets[0]->setRect({y + 1, x + 1, dialogHeight - 2, dialogWidth - 2});
}

void ConnectionFailedScreen::resize() {
    layout();
}

request::UIRequest ConnectionFailedScreen::handleInput(int key) {
    const InputResult result = handleFocusedInput(key);
    if (!std::holds_alternative<request::None>(result.request)) {
        return result.request;
    }

    if (result.handled) {
        return request::None{};
    }

    switch (key) {
        case KEY_UP:
        case KEY_LEFT:
        case KEY_BTAB:
            focusPrev();
            return request::None{};

        case KEY_DOWN:
        case KEY_RIGHT:
        case '\t':
            focusNext();
            return request::None{};
    }

    return request::None{};
}

}  // namespace plague::ui
