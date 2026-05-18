#include "ScreenShared.hpp"

#include <algorithm>
#include <iomanip>
#include <numeric>
#include <sstream>

namespace plague::ui {

namespace {

double averageMedicine(const std::vector<Country> & countries) {
    if (countries.empty()) {
        return 0.0;
    }

    const int total = std::accumulate(
        countries.begin(),
        countries.end(),
        0,
        [](int sum, const Country & country) {
            return sum + countryParamsForName(country.name).medicine;
        }
    );
    return static_cast<double>(total) / static_cast<double>(countries.size());
}

double averageGovernmentReaction(const std::vector<Country> & countries) {
    if (countries.empty()) {
        return 0.0;
    }

    const int total = std::accumulate(
        countries.begin(),
        countries.end(),
        0,
        [](int sum, const Country & country) {
            return sum + countryParamsForName(country.name).governmentReaction;
        }
    );
    return static_cast<double>(total) / static_cast<double>(countries.size());
}

std::vector<const Country *> topMedicineCountries(const std::vector<Country> & countries) {
    std::vector<const Country *> result;
    result.reserve(countries.size());
    for (const Country & country : countries) {
        result.push_back(&country);
    }

    std::sort(result.begin(), result.end(), [](const Country * lhs, const Country * rhs) {
        const CountryParams lhsParams = countryParamsForName(lhs->name);
        const CountryParams rhsParams = countryParamsForName(rhs->name);
        if (lhsParams.medicine != rhsParams.medicine) {
            return lhsParams.medicine > rhsParams.medicine;
        }
        return lhsParams.governmentReaction > rhsParams.governmentReaction;
    });

    if (result.size() > 5) {
        result.resize(5);
    }
    return result;
}

}

CureInfoScreen::CureInfoScreen(Config & cfg, Window & win)
    : InfoNavigationScreen(cfg, win) {
    auto info = std::make_unique<Info>(win_, "");
    cureInfo_ = info.get();
    widgets.push_back(std::make_unique<FrameDecorator>(
        win_,
        std::move(info)
    ));
    updateInfo();
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

void CureInfoScreen::updateSnapshot(const GameSnapshot & snapshot) {
    snapshot_ = snapshot;
    updateInfo();
}

void CureInfoScreen::updateInfo() {
    if (cureInfo_ == nullptr) {
        return;
    }

    const std::uint64_t total = totalPopulation(snapshot_.countries);
    const std::uint64_t infected = totalInfectedCount(snapshot_.countries);
    const std::uint64_t dead = totalDeadCount(snapshot_.countries);
    const double infectedPercent = total == 0
        ? 0.0
        : static_cast<double>(infected) * 100.0 / static_cast<double>(total);
    const double deadPercent = total == 0
        ? 0.0
        : static_cast<double>(dead) * 100.0 / static_cast<double>(total);

    std::ostringstream out;
    out << "Cure overview"
        << "\n\nProgress: " << std::fixed << std::setprecision(1) << snapshot_.cureProgress << "%"
        << "\nRemaining: " << std::max(0.0, 100.0 - snapshot_.cureProgress) << "%"
        << "\n\nGlobal pressure"
        << "\nInfected / exposed: " << formatCount(infected) << " (" << std::setprecision(2) << infectedPercent << "%)"
        << "\nDead: " << formatCount(dead) << " (" << std::setprecision(2) << deadPercent << "%)"
        << "\n\nHealthcare readiness"
        << "\nAverage medicine: " << std::setprecision(2) << averageMedicine(snapshot_.countries) << " / 5"
        << "\nAverage government reaction: " << averageGovernmentReaction(snapshot_.countries) << " / 5"
        << "\n\nBest prepared countries";

    const std::vector<const Country *> prepared = topMedicineCountries(snapshot_.countries);
    if (prepared.empty()) {
        out << "\nNo country data";
    } else {
        for (const Country * country : prepared) {
            const CountryParams params = countryParamsForName(country->name);
            out << "\n- " << country->name
                << " | medicine " << params.medicine
                << " | reaction " << params.governmentReaction
                << " | borders " << (country->bordersClosed ? "closed" : "open");
        }
    }

    cureInfo_->changeText(out.str());
}

}  // namespace plague::ui
