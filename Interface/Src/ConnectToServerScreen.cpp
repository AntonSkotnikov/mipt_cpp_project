#include "ScreenShared.hpp"

namespace plague::ui {

ConnectToServerScreen::ConnectToServerScreen(Config & cfg, Window & win) : Screen(cfg, win) {
    auto addressInput = std::make_unique<TextInput>(win_);
    auto portInput = std::make_unique<TextInput>(win_);

    TextInput * address = addressInput.get();
    TextInput * port = portInput.get();

    widgets.push_back(std::make_unique<LabelDecorator>(
        win_,
        std::make_unique<FrameDecorator>(win_, std::move(addressInput)),
        "Address"
    ));

    widgets.push_back(std::make_unique<LabelDecorator>(
        win_,
        std::make_unique<FrameDecorator>(win_, std::move(portInput)),
        "Port"
    ));

    auto menuWidget = std::make_unique<Menu>(win_);
    menuWidget->addButton("Connect", [this, address, port]() -> request::UIRequest {
        cfg_.lastConnectAddr = address->getText();
        cfg_.lastConnectPort = port->getText();
        return request::ConnectInfo{
            request::Connect::Connect,
            cfg_.lastConnectAddr,
            cfg_.lastConnectPort
        };
    });

    menuWidget->addButton("Back", []() -> request::UIRequest {
        return request::ConnectInfo{request::Connect::Back, "", ""};
    });

    widgets.push_back(std::move(menuWidget));

    layout();
    focusFirst();
}


void ConnectToServerScreen::layout() {
    if (widgets.size() < 2) return;

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

request::UIRequest ConnectToServerScreen::handleInput(int key) {
    const InputResult result = handleFocusedInput(key);
    if (!std::holds_alternative<request::None>(result.request)) {
        return result.request;
    }

    if (result.handled) {
        return request::None{};
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
    }

    return request::None{};
}

}  // namespace plague::ui
