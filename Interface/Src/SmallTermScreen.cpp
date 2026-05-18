#include "ScreenShared.hpp"

namespace plague::ui {

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

request::UIRequest SmallTermScreen::handleInput(int key) {
    (void)key;
    return request::None{};
}

}  // namespace plague::ui
