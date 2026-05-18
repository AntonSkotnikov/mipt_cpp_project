#include "ScreenShared.hpp"

namespace plague::ui {

CountryScreen::CountryScreen(Config & cfg, Window & win)
    : InfoNavigationScreen(cfg, win) {
    auto menu = std::make_unique<Menu>(win_);
    countryMenu_ = menu.get();
    for (std::size_t i = 0; i < lowMapCountries.size(); i++) {
        menu->addButton(lowMapCountries[i], []() -> request::UIRequest {
            return request::None{};
        });
    }

    widgets.push_back(std::make_unique<LabelDecorator>(
        win_,
        std::make_unique<FrameDecorator>(win_, std::move(menu)),
        "Countries"
    ));

    auto info = std::make_unique<Info>(win_, "");
    countryInfo_ = info.get();
    widgets.push_back(std::make_unique<LabelDecorator>(
        win_,
        std::make_unique<FrameDecorator>(win_, std::move(info)),
        "Description"
    ));

    updateSelectedCountryInfo();
    layout();
}

void CountryScreen::layout() {
    layoutNavigation();
    if (widgets.size() <= bodyWidgetStart + 1) return;

    const Rect body = bodyRect();
    const int gap = 1;
    const int menuWidth = std::min(36, std::max(24, body.width / 3));
    const int infoX = body.x + menuWidth + gap;
    const int infoWidth = std::max(1, body.x + body.width - infoX);

    widgets[bodyWidgetStart]->setRect({
        body.y + 1,
        body.x,
        std::max(1, body.height - 1),
        menuWidth - 1
    });
    widgets[bodyWidgetStart + 1]->setRect({
        body.y + 1,
        infoX,
        std::max(1, body.height - 1),
        infoWidth
    });
}

void CountryScreen::updateSelectedCountryInfo() {
    if (countryMenu_ == nullptr || countryInfo_ == nullptr) {
        return;
    }

    const std::size_t index = std::min(countryMenu_->selectedIndex(), lowMapCountries.size() - 1);
    countryInfo_->changeText(std::string(lowMapCountries[index]) + "\n\nCountry information\n\nWIP");
}

void CountryScreen::afterHandledInput() {
    updateSelectedCountryInfo();
}

void CountryScreen::resize() {
    layout();
}

}  // namespace plague::ui
