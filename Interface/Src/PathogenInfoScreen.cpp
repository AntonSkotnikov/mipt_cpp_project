#include "ScreenShared.hpp"

namespace plague::ui {

PathogenInfoScreen::PathogenInfoScreen(Config & cfg, Window & win)
    : InfoNavigationScreen(cfg, win) {
    widgets.push_back(std::make_unique<FrameDecorator>(
        win_,
        std::make_unique<Info>(win_, "Pathogen information\n\nWIP")
    ));
    layout();
}

void PathogenInfoScreen::layout() {
    layoutNavigation();
    if (widgets.size() > bodyWidgetStart) {
        widgets[bodyWidgetStart]->setRect(bodyRect());
    }
}

void PathogenInfoScreen::resize() {
    layout();
}

}  // namespace plague::ui
