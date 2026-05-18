#pragma once

#include "GameTypes.hpp"
#include "MapParser.hpp"
#include "Screen.hpp"
#include "Settings.hpp"
#include "SimulationTypes.hpp"
#include "UIRequest.hpp"
#include "UI_ClientAPI.hpp"
#include "Upgrade.hpp"
#include "Widget.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iterator>
#include <ncurses.h>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace plague::ui {

constexpr std::array<const char *, 29> lowMapCountries = {
    "AUSTRALIA",
    "BELARUS",
    "BRAZIL",
    "CANADA",
    "CHINA",
    "EAST",
    "GREENLAND",
    "ICELAND",
    "INDIA",
    "JAPAN",
    "KAZAKHSTAN",
    "M AFRICA",
    "MADAGASCAR",
    "MEXICO",
    "MIDDLE EAST",
    "MONGOLIA",
    "N AFRICA",
    "N SOUTH AMERICA",
    "NEW ZEALAND",
    "OCEANIA",
    "RUSSIA",
    "S AFRICA",
    "SCANDINAVIA",
    "SW SOUTH AMERICA",
    "TURKEY",
    "UK",
    "UKRAINE",
    "USA",
    "W EUROPE"
};

constexpr std::size_t countryWidgetCount = lowMapCountries.size();
constexpr std::size_t mapFrameIndex = 0;
constexpr std::size_t countryWidgetStart = mapFrameIndex + 1;
constexpr std::size_t gameStatWidgetStart = countryWidgetStart + countryWidgetCount;
constexpr std::size_t gameDnaIndex = gameStatWidgetStart;
constexpr std::size_t gameIllIndex = gameStatWidgetStart + 1;
constexpr std::size_t gameWorldInfoIndex = gameStatWidgetStart + 2;
constexpr std::size_t gameDeadIndex = gameStatWidgetStart + 3;
constexpr std::size_t gameCureIndex = gameStatWidgetStart + 4;
constexpr std::size_t gameUpgradeIndex = gameStatWidgetStart + 5;
constexpr std::size_t gameTickerIndex = gameStatWidgetStart + 6;
constexpr std::size_t gameDayIndex = gameStatWidgetStart + 7;
constexpr std::size_t gameWorldButtonIndex = gameStatWidgetStart + 8;
constexpr std::array<std::size_t, 2> gameActionButtonIndices = {
    gameUpgradeIndex,
    gameWorldButtonIndex
};
constexpr std::size_t choosingStatusIndex = 0;
constexpr std::size_t choosingMenuIndex = 1;
constexpr std::size_t choosingDescriptionIndex = 2;
constexpr std::size_t choosingChangeSideIndex = 3;
constexpr std::size_t choosingReadyIndex = 4;
constexpr std::array<std::size_t, 2> choosingBottomButtonIndices = {
    choosingChangeSideIndex,
    choosingReadyIndex
};
constexpr int defaultColorPair = 0;
constexpr int blueColorPair = 2;
constexpr int greenColorPair = 3;
constexpr std::size_t infoTabCount = 4;
constexpr std::array<request::Game, infoTabCount> infoTabRequests = {
    request::Game::Info,
    request::Game::Cure,
    request::Game::World,
    request::Game::News
};
constexpr std::array<const char *, infoTabCount> infoTabLabels = {
    "Pathogen",
    "Cure",
    "Countries",
    "News"
};
constexpr std::array<request::Game, 3> upgradeTabRequests = {
    request::Game::Transmission,
    request::Game::Clinic,
    request::Game::Abilities
};
constexpr std::array<const char *, 3> upgradeTabLabels = {
    "Transmission",
    "Clinic",
    "Abilities"
};

struct CountryParamsPresentation {
    const char * name;
    CountryParams params;
};

constexpr std::array<CountryParamsPresentation, 29> countryParamsPresentations = {{
    {"AUSTRALIA",          {5, 4, 5, 4}},
    {"BELARUS",            {3, 2, 3, 3}},
    {"BRAZIL",             {2, 5, 4, 2}},
    {"CANADA",             {5, 1, 4, 4}},
    {"CHINA",              {3, 3, 5, 5}},
    {"EAST",               {2, 4, 3, 2}},
    {"GREENLAND",          {3, 1, 1, 3}},
    {"ICELAND",            {4, 1, 2, 4}},
    {"INDIA",              {2, 5, 4, 2}},
    {"JAPAN",              {5, 3, 5, 5}},
    {"KAZAKHSTAN",         {3, 2, 3, 3}},
    {"M AFRICA",           {2, 5, 2, 2}},
    {"MADAGASCAR",         {2, 5, 2, 2}},
    {"MEXICO",             {3, 4, 4, 3}},
    {"MIDDLE EAST",        {3, 5, 4, 3}},
    {"MONGOLIA",           {2, 2, 2, 3}},
    {"N AFRICA",           {2, 5, 3, 2}},
    {"N SOUTH AMERICA",    {2, 5, 3, 2}},
    {"NEW ZEALAND",        {5, 3, 3, 5}},
    {"OCEANIA",            {2, 5, 2, 2}},
    {"RUSSIA",             {3, 2, 3, 3}},
    {"S AFRICA",           {3, 4, 3, 3}},
    {"SCANDINAVIA",        {5, 1, 4, 5}},
    {"SW SOUTH AMERICA",   {3, 3, 3, 3}},
    {"TURKEY",             {3, 4, 4, 3}},
    {"UK",                 {5, 2, 5, 4}},
    {"UKRAINE",            {3, 2, 3, 3}},
    {"USA",                {5, 3, 5, 3}},
    {"W EUROPE",           {5, 3, 5, 4}}
}};

inline CountryParams countryParamsForName(std::string_view name) {
    const auto it = std::find_if(
        countryParamsPresentations.begin(),
        countryParamsPresentations.end(),
        [name](const CountryParamsPresentation & item) {
            return item.name == name;
        }
    );

    return it == countryParamsPresentations.end()
        ? CountryParams{3, 3, 3, 3}
        : it->params;
}

inline std::string formatCount(std::uint64_t value) {
    std::string text = std::to_string(value);
    for (int pos = static_cast<int>(text.size()) - 3; pos > 0; pos -= 3) {
        text.insert(static_cast<std::size_t>(pos), " ");
    }
    return text;
}

inline std::uint64_t populationCount(double value) {
    return static_cast<std::uint64_t>(std::max(0.0, value));
}

inline std::uint64_t countryInfectedCount(const Country & country) {
    return populationCount(country.pop.exposed + country.pop.infected);
}

inline std::uint64_t countryDeadCount(const Country & country) {
    return populationCount(country.pop.dead);
}

inline std::uint64_t countryAliveCount(const Country & country) {
    return populationCount(country.pop.alive());
}

inline std::uint64_t totalPopulation(const std::vector<Country> & countries) {
    std::uint64_t total = 0;
    for (const Country & country : countries) {
        total += populationCount(country.pop.initial);
    }
    return total;
}

inline std::uint64_t totalAliveCount(const std::vector<Country> & countries) {
    std::uint64_t total = 0;
    for (const Country & country : countries) {
        total += countryAliveCount(country);
    }
    return total;
}

inline std::uint64_t totalInfectedCount(const std::vector<Country> & countries) {
    std::uint64_t total = 0;
    for (const Country & country : countries) {
        total += countryInfectedCount(country);
    }
    return total;
}

inline std::uint64_t totalDeadCount(const std::vector<Country> & countries) {
    std::uint64_t total = 0;
    for (const Country & country : countries) {
        total += countryDeadCount(country);
    }
    return total;
}

inline const Country * findCountry(const std::vector<Country> & countries, std::string_view name) {
    const auto it = std::find_if(countries.begin(), countries.end(),
        [name](const Country & country) {
            return country.name == name;
        });

    return it == countries.end() ? nullptr : &*it;
}

inline Rect innerRect(Rect outer) {
    return {
        outer.y + 1,
        outer.x + 1,
        std::max(1, outer.height - 2),
        std::max(1, outer.width - 2)
    };
}

inline Rect countryBounds(const std::vector<SymbolOnScreen> & symbols) {
    if (symbols.empty()) {
        return {};
    }

    int minY = symbols.front().y;
    int maxY = symbols.front().y;
    int minX = symbols.front().x;
    int maxX = symbols.front().x;

    for (const SymbolOnScreen & symbol : symbols) {
        minY = std::min(minY, symbol.y);
        maxY = std::max(maxY, symbol.y);
        minX = std::min(minX, symbol.x);
        maxX = std::max(maxX, symbol.x);
    }

    return {
        minY,
        minX,
        maxY - minY + 1,
        maxX - minX + 1
    };
}

inline bool validCountryBounds(Rect bounds) {
    return bounds.height > 0 && bounds.width > 0;
}

inline int centerY(Rect bounds) {
    return bounds.y + bounds.height / 2;
}

inline int centerX(Rect bounds) {
    return bounds.x + bounds.width / 2;
}

inline bool isCountryInDirection(Rect current,
                          Rect candidate,
                          int key) {
    switch (key) {
        case KEY_LEFT:
            return centerX(candidate) < centerX(current);
        case KEY_RIGHT:
            return centerX(candidate) > centerX(current);
        case KEY_UP:
            return centerY(candidate) < centerY(current);
        case KEY_DOWN:
            return centerY(candidate) > centerY(current);
    }

    return false;
}

inline int directionalScore(Rect current, Rect candidate, int key) {
    const int dx = centerX(candidate) - centerX(current);
    const int dy = centerY(candidate) - centerY(current);
    const int primary = (key == KEY_LEFT || key == KEY_RIGHT) ? std::abs(dx) : std::abs(dy);
    const int secondary = (key == KEY_LEFT || key == KEY_RIGHT) ? std::abs(dy) : std::abs(dx);

    return primary * primary + secondary * secondary * 4;
}

template <std::size_t N>
std::size_t wrappedIndexNear(const std::array<std::size_t, N> & indices,
                             std::size_t focusedIndex,
                             int direction) {
    const auto it = std::find(indices.begin(), indices.end(), focusedIndex);
    const std::size_t current = it == indices.end()
        ? 0
        : static_cast<std::size_t>(std::distance(indices.begin(), it));

    if (direction < 0) {
        return current == 0 ? N - 1 : current - 1;
    }

    return (current + 1) % N;
}

struct SubtypePresentation {
    PlayerSubtype subtype;
    const char * label;
    const char * description;
};

const std::array<SubtypePresentation, 1> humanitySubtypes = {{
    {
        HumanitySubtype::ResearchInstitute,
        "Research Institute",
        "Humanity focuses on containment, research, and survival.\n\n"
        "Research Institute starts with disciplined laboratories and strong medical coordination. "
        "Your priorities are to slow global spread, keep borders under control, and push cure progress "
        "before the pathogen overwhelms healthcare systems.\n\n"
        "Watch infection pressure, country medicine ratings, government reaction, and cure progress. "
        "Use upgrades to improve response speed and reduce the pathogen's room to grow."
    }
}};

const std::array<SubtypePresentation, 1> pathogenSubtypes = {{
    {
        PathogenSubtype::Virus,
        "Virus",
        "The pathogen wins by spreading faster than humanity can understand and contain it.\n\n"
        "Virus is a direct, adaptable infection profile. It benefits from early expansion, pressure on "
        "dense countries, and careful timing before borders close or cure progress accelerates.\n\n"
        "Watch infected countries, transport links, closed borders, and cure pressure. Use upgrades to "
        "strengthen transmission, disrupt clinics, and survive hostile climates."
    }
}};

inline std::size_t subtypeCountFor(PlayerRole role) {
    return role == PlayerRole::Humanity ? humanitySubtypes.size() : pathogenSubtypes.size();
}

inline std::size_t maxSubtypeCount() {
    return std::max(humanitySubtypes.size(), pathogenSubtypes.size());
}

inline const SubtypePresentation & subtypeAt(PlayerRole role, std::size_t index) {
    if (role == PlayerRole::Humanity) {
        return humanitySubtypes[std::min(index, humanitySubtypes.size() - 1)];
    }

    return pathogenSubtypes[std::min(index, pathogenSubtypes.size() - 1)];
}

inline const SubtypePresentation & presentationFor(PlayerRole role, PlayerSubtype subtype) {
    const std::size_t count = subtypeCountFor(role);

    for (std::size_t i = 0; i < count; i++) {
        const SubtypePresentation & item = subtypeAt(role, i);
        if (item.subtype == subtype) {
            return item;
        }
    }

    return subtypeAt(role, 0);
}

inline const char * roleName(PlayerRole role) {
    return role == PlayerRole::Humanity ? "Humanity" : "Pathogen";
}

inline std::size_t upgradeTabIndex(UpgradeCategory category) {
    switch (category) {
        case UpgradeCategory::Transmission: return 0;
        case UpgradeCategory::Clinic:       return 1;
        case UpgradeCategory::Abilities:    return 2;
    }

    return 0;
}

inline bool containsUpgrade(const std::vector<UpgradeId> & upgrades, const UpgradeId & id) {
    return std::find(upgrades.begin(), upgrades.end(), id) != upgrades.end();
}

inline std::string titleFromUpgradeId(const UpgradeId & id) {
    std::string title;
    title.reserve(id.size());
    bool capitalizeNext = true;

    for (const char ch : id) {
        if (ch == '_' || ch == '-') {
            title.push_back(' ');
            capitalizeNext = true;
            continue;
        }

        title.push_back(capitalizeNext ? static_cast<char>(std::toupper(static_cast<unsigned char>(ch))) : ch);
        capitalizeNext = false;
    }

    return title.empty() ? "Unnamed upgrade" : title;
}

inline UpgradeDefinition normalizedUpgrade(UpgradeDefinition upgrade) {
    if (upgrade.title.empty()) {
        upgrade.title = titleFromUpgradeId(upgrade.id);
    }
    return upgrade;
}

inline std::vector<UpgradeListItem> upgradeItemsFor(const GameSnapshot & snapshot, UpgradeCategory category) {
    std::vector<UpgradeListItem> items;

    for (const UpgradeDefinition & source : snapshot.availableUpgrades) {
        UpgradeDefinition upgrade = normalizedUpgrade(source);
        if (upgrade.category != category) {
            continue;
        }

        items.push_back({
            upgrade,
            dependenciesSatisfied(upgrade, snapshot.purchasedUpgrades),
            containsUpgrade(snapshot.purchasedUpgrades, upgrade.id)
        });
    }

    return items;
}

}  // namespace plague::ui
