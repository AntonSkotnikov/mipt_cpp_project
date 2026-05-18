#include "ScreenShared.hpp"

namespace plague::ui {

NewsScreen::NewsScreen(Config & cfg, Window & win)
    : InfoNavigationScreen(cfg, win) {
    layout();
}

void NewsScreen::layout() {
    layoutNavigation();
}

void NewsScreen::resize() {
    layout();
}

}  // namespace plague::ui
