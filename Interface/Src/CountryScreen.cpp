#include "ScreenShared.hpp"

#include <iomanip>
#include <sstream>

namespace plague::ui {

namespace {

std::string percentText(double value, double total) {
    const double percent = total <= 0.0 ? 0.0 : value * 100.0 / total;
    std::ostringstream out;
    out << std::fixed << std::setprecision(2) << percent << "%";
    return out.str();
}

const char * climateLabel(int climate) {
    switch (climate) {
        case 1: return "cold";
        case 2: return "cool";
        case 3: return "temperate";
        case 4: return "warm";
        case 5: return "tropical";
        default: return "unknown";
    }
}

std::string countryDetailsText(
    std::string_view displayName,
    const Country & country,
    const CountryParams & params,
    bool highlighted
) {
    std::ostringstream out;
    out << displayName
        << "\n\nPopulation"
        << "\nInitial: " << formatCount(populationCount(country.pop.initial))
        << "\nAlive: " << formatCount(countryAliveCount(country))
        << "\nSusceptible: " << formatCount(populationCount(country.pop.susceptible))
        << " (" << percentText(country.pop.susceptible, country.pop.initial) << ")"
        << "\nExposed: " << formatCount(populationCount(country.pop.exposed))
        << " (" << percentText(country.pop.exposed, country.pop.initial) << ")"
        << "\nInfected: " << formatCount(populationCount(country.pop.infected))
        << " (" << percentText(country.pop.infected, country.pop.initial) << ")"
        << "\nRecovered: " << formatCount(populationCount(country.pop.recovered))
        << " (" << percentText(country.pop.recovered, country.pop.initial) << ")"
        << "\nDead: " << formatCount(countryDeadCount(country))
        << " (" << percentText(country.pop.dead, country.pop.initial) << ")"
        << "\n\nCountry profile"
        << "\nMedicine: " << params.medicine << " / 5"
        << "\nClimate: " << params.climate << " / 5 (" << climateLabel(params.climate) << ")"
        << "\nUrbanization: " << params.urbanization << " / 5"
        << "\nGovernment reaction: " << params.governmentReaction << " / 5"
        << "\n\nMobility"
        << "\nBorder openness: " << std::fixed << std::setprecision(1) << country.borderOpenness
        << "\nBorders: " << (country.bordersClosed ? "closed" : "open")
        << "\nEvent highlight: " << (highlighted ? "yes" : "no");
    return out.str();
}

}

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
    const std::string_view countryName = lowMapCountries[index];
    if (const Country * country = findCountry(snapshot_.countries, countryName)) {
        const bool highlighted = index < snapshot_.highlightedCountries.size() && snapshot_.highlightedCountries[index];
        countryInfo_->changeText(countryDetailsText(countryName, *country, countryParamsForName(countryName), highlighted));
        return;
    }

    countryInfo_->changeText(
        std::string(countryName) +
        "\n\nNo live country data yet.\nWaiting for the next game snapshot."
    );
}

void CountryScreen::afterHandledInput() {
    updateSelectedCountryInfo();
}

void CountryScreen::resize() {
    layout();
}

void CountryScreen::updateSnapshot(const GameSnapshot & snapshot) {
    snapshot_ = snapshot;
    updateSelectedCountryInfo();
}

}  // namespace plague::ui
