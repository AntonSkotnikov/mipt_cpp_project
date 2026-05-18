#include "ScreenShared.hpp"

namespace plague::ui {

InfoNavigationScreen::InfoNavigationScreen(Config & cfg, Window & win)
    : Screen(cfg, win) {

    for (std::size_t i = 0; i < infoTabCount; i++) {
        std::string label = infoTabLabels[i];

        widgets.push_back(std::make_unique<FrameDecorator>(
            win_,
            std::make_unique<Button>(win_, std::move(label), [i]() -> request::UIRequest {
                return infoTabRequests[i];
            })
        ));
    }

    layoutNavigation();
    focusFirst();
}

void InfoNavigationScreen::layoutNavigation() {
    if (widgets.size() < infoTabCount) return;

    const int padding = win_.bordered() ? 2 : 1;
    const int contentX = padding;
    const int contentY = padding;
    const int contentWidth = std::max(1, win_.width() - padding * 2);
    const int gap = 1;
    const int tabHeight = 3;
    const int tabWidth = std::max(8, (contentWidth - gap * static_cast<int>(infoTabCount - 1)) / static_cast<int>(infoTabCount));

    for (std::size_t i = 0; i < infoTabCount; i++) {
        const int x = contentX + static_cast<int>(i) * (tabWidth + gap);
        const int width = i == infoTabCount - 1
            ? std::max(1, contentX + contentWidth - x)
            : tabWidth;
        widgets[i]->setRect(innerRect({contentY, x, tabHeight, width}));
    }
}

Rect InfoNavigationScreen::bodyRect() const {
    const int padding = win_.bordered() ? 2 : 1;
    const int contentX = padding;
    const int contentY = padding;
    const int contentWidth = std::max(1, win_.width() - padding * 2);
    const int contentHeight = std::max(1, win_.height() - padding * 2);
    const int tabHeight = 3;
    const int gap = 1;

    return {
        contentY + tabHeight + gap + 1,
        contentX + 1,
        std::max(1, contentHeight - tabHeight - gap - 2),
        std::max(1, contentWidth - 2)
    };
}

void InfoNavigationScreen::resize() {
    layoutNavigation();
    layoutBody();
}

request::UIRequest InfoNavigationScreen::handleInput(int key) {
    if (key == 27) {
        return request::Game::Back;
    }

    const InputResult result = handleFocusedInput(key);
    if (!std::holds_alternative<request::None>(result.request)) {
        return result.request;
    }

    if (result.handled) {
        afterHandledInput();
        return request::None{};
    }

    switch (key) {
        case KEY_LEFT:
        case KEY_UP:
        case KEY_BTAB:
            focusPrev();
            afterHandledInput();
            return request::None{};

        case KEY_RIGHT:
        case KEY_DOWN:
        case '\t':
            focusNext();
            afterHandledInput();
            return request::None{};
    }

    return request::None{};
}

}  // namespace plague::ui
