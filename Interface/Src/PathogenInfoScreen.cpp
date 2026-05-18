#include "ScreenShared.hpp"

#include <algorithm>
#include <iomanip>
#include <numeric>
#include <sstream>

namespace plague::ui {

namespace {

std::string percentLine(std::string label, std::uint64_t value, std::uint64_t total) {
    const double percent = total == 0
        ? 0.0
        : static_cast<double>(value) * 100.0 / static_cast<double>(total);

    std::ostringstream out;
    out << label << ": " << formatCount(value) << " (" << std::fixed << std::setprecision(2) << percent << "%)";
    return out.str();
}

std::size_t countCountriesWithInfections(const std::vector<Country> & countries) {
    return static_cast<std::size_t>(std::count_if(countries.begin(), countries.end(), [](const Country & country) {
        return countryInfectedCount(country) > 0;
    }));
}

std::size_t countExtinctCountries(const std::vector<Country> & countries) {
    return static_cast<std::size_t>(std::count_if(countries.begin(), countries.end(), [](const Country & country) {
        return populationCount(country.pop.alive()) == 0 && populationCount(country.pop.initial) > 0;
    }));
}

std::size_t countClosedCountries(const std::vector<Country> & countries) {
    return static_cast<std::size_t>(std::count_if(countries.begin(), countries.end(), [](const Country & country) {
        return country.bordersClosed;
    }));
}

std::string upgradeSummary(const GameSnapshot & snapshot) {
    std::ostringstream out;
    out << "Purchased upgrades: " << snapshot.purchasedUpgrades.size();
    if (!snapshot.purchasedUpgrades.empty()) {
        out << "\n";
        for (const UpgradeId & id : snapshot.purchasedUpgrades) {
            out << "- " << titleFromUpgradeId(id) << "\n";
        }
    }
    return out.str();
}

}

PathogenInfoScreen::PathogenInfoScreen(Config & cfg, Window & win)
    : InfoNavigationScreen(cfg, win) {
    auto info = std::make_unique<Info>(win_, "");
    pathogenInfo_ = info.get();
    widgets.push_back(std::make_unique<FrameDecorator>(
        win_,
        std::move(info)
    ));
    updateInfo();
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

void PathogenInfoScreen::updateSnapshot(const GameSnapshot & snapshot) {
    snapshot_ = snapshot;
    updateInfo();
}

void PathogenInfoScreen::updateInfo() {
    if (pathogenInfo_ == nullptr) {
        return;
    }

    const std::uint64_t total = totalPopulation(snapshot_.countries);
    const std::uint64_t alive = totalAliveCount(snapshot_.countries);
    const std::uint64_t infected = totalInfectedCount(snapshot_.countries);
    const std::uint64_t dead = totalDeadCount(snapshot_.countries);
    const std::uint64_t recovered = static_cast<std::uint64_t>(std::max(0.0, std::accumulate(
        snapshot_.countries.begin(),
        snapshot_.countries.end(),
        0.0,
        [](double sum, const Country & country) {
            return sum + country.pop.recovered;
        }
    )));

    std::ostringstream out;
    out << "Pathogen overview"
        << "\n\nRole: " << roleName(snapshot_.playerInfo.role)
        << "\nDay: " << snapshot_.day
        << "\nPoints: " << snapshot_.playerInfo.points
        << "\n\nSpread"
        << "\n" << percentLine("Infected / exposed", infected, total)
        << "\n" << percentLine("Recovered", recovered, total)
        << "\n" << percentLine("Dead", dead, total)
        << "\nAlive: " << formatCount(alive)
        << "\nAffected countries: " << countCountriesWithInfections(snapshot_.countries)
        << " / " << snapshot_.countries.size()
        << "\nExtinct countries: " << countExtinctCountries(snapshot_.countries)
        << "\nClosed-border countries: " << countClosedCountries(snapshot_.countries)
        << "\n\nCure pressure"
        << "\nCure progress: " << std::fixed << std::setprecision(1) << snapshot_.cureProgress << "%"
        << "\n\n" << upgradeSummary(snapshot_);

    pathogenInfo_->changeText(out.str());
}

}  // namespace plague::ui
