#include "ScreenShared.hpp"

namespace plague::ui {

NewsScreen::NewsScreen(Config & cfg, Window & win)
    : InfoNavigationScreen(cfg, win) {
    auto newsInfo = std::make_unique<Info>(win_, "No news yet");
    newsInfo_ = newsInfo.get();
    widgets.push_back(std::make_unique<LabelDecorator>(
        win_,
        std::make_unique<FrameDecorator>(win_, std::move(newsInfo)),
        "News"
    ));

    updateNews();
    layout();
}

void NewsScreen::layout() {
    layoutNavigation();
    layoutBody();
}

void NewsScreen::layoutBody() {
    if (widgets.size() <= bodyWidgetStart) {
        return;
    }

    widgets[bodyWidgetStart]->setRect(bodyRect());
}

void NewsScreen::resize() {
    layout();
}

void NewsScreen::updateSnapshot(const GameSnapshot & snapshot) {
    snapshot_ = snapshot;
    updateNews();
}

void NewsScreen::updateNews() {
    if (newsInfo_ == nullptr) {
        return;
    }

    if (snapshot_.news.empty()) {
        newsInfo_->changeText("No news yet");
        return;
    }

    std::string text;
    for (std::size_t i = 0; i < snapshot_.news.size(); i++) {
        if (i > 0) {
            text += "\n\n";
        }
        text += snapshot_.news[i];
    }

    newsInfo_->changeText(std::move(text));
}

}  // namespace plague::ui
