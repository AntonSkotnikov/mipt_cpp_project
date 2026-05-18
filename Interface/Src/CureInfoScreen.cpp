#include "ScreenShared.hpp"

namespace plague::ui {

CureInfoScreen::CureInfoScreen(Config & cfg, Window & win)
    : InfoNavigationScreen(cfg, win) {
    widgets.push_back(std::make_unique<FrameDecorator>(
        win_,
        std::make_unique<Info>(win_, "Cure information\n\nWIP")
    ));
    layout();
}

void CureInfoScreen::layout() {
    layoutNavigation();
    if (widgets.size() > bodyWidgetStart) {
        widgets[bodyWidgetStart]->setRect(bodyRect());
    }
}

void CureInfoScreen::resize() {
    layout();
}

}  // namespace plague::ui
