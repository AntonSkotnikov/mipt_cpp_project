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
    renderNews();
}

void NewsScreen::updateSnapshot(const GameSnapshot & snapshot) {
    snapshot_ = snapshot;
    updateNews();
}

request::UIRequest NewsScreen::handleInput(int key) {
    if (key == 27) {
        return request::Game::Back;
    }

    switch (key) {
        case KEY_UP:
            scrollNews(-1);
            return request::None{};

        case KEY_DOWN:
            scrollNews(1);
            return request::None{};

        case KEY_PPAGE:
            scrollNews(-std::max(1, bodyRect().height - 2));
            return request::None{};

        case KEY_NPAGE:
            scrollNews(std::max(1, bodyRect().height - 2));
            return request::None{};

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

void NewsScreen::updateNews() {
    if (newsInfo_ == nullptr) {
        return;
    }

    newsLines_.clear();
    if (snapshot_.news.empty()) {
        scrollOffset_ = 0;
        newsLines_.push_back("No news yet");
        renderNews();
        return;
    }

    for (std::size_t i = 0; i < snapshot_.news.size(); i++) {
        if (i > 0) {
            newsLines_.push_back("");
        }
        newsLines_.push_back(snapshot_.news[i]);
    }

    const std::size_t visible = static_cast<std::size_t>(std::max(1, bodyRect().height - 2));
    if (scrollOffset_ + visible > newsLines_.size()) {
        scrollOffset_ = newsLines_.size() > visible ? newsLines_.size() - visible : 0;
    }

    renderNews();
}

void NewsScreen::renderNews() {
    if (newsInfo_ == nullptr) {
        return;
    }

    const std::size_t visible = static_cast<std::size_t>(std::max(1, bodyRect().height - 2));
    const std::size_t maxOffset = newsLines_.size() > visible ? newsLines_.size() - visible : 0;
    scrollOffset_ = std::min(scrollOffset_, maxOffset);

    std::string text;
    const std::size_t end = std::min(newsLines_.size(), scrollOffset_ + visible);
    for (std::size_t i = scrollOffset_; i < end; i++) {
        if (i > scrollOffset_) {
            text += '\n';
        }
        text += newsLines_[i];
    }

    if (maxOffset > 0) {
        text += "\n\nShowing ";
        text += std::to_string(scrollOffset_ + 1);
        text += "-";
        text += std::to_string(end);
        text += " of ";
        text += std::to_string(newsLines_.size());
    }

    newsInfo_->changeText(std::move(text));
}

void NewsScreen::scrollNews(int delta) {
    const std::size_t visible = static_cast<std::size_t>(std::max(1, bodyRect().height - 2));
    const std::size_t maxOffset = newsLines_.size() > visible ? newsLines_.size() - visible : 0;

    if (delta < 0) {
        const std::size_t amount = static_cast<std::size_t>(-delta);
        scrollOffset_ = amount > scrollOffset_ ? 0 : scrollOffset_ - amount;
    } else {
        scrollOffset_ = std::min(maxOffset, scrollOffset_ + static_cast<std::size_t>(delta));
    }

    renderNews();
}

}  // namespace plague::ui
