#pragma once

#include "GameTypes.hpp"
#include <string>
#include <vector>

namespace plague {

using UpgradeId = std::string;

enum class UpgradeCategory {
    Transmission,
    Clinic,
    Abilities
};

struct UpgradeDefinition {
    UpgradeId id;
    UpgradeCategory category;
    std::string title;
    UpgradePointType cost = 0;
    std::string description;
    std::vector<UpgradeId> dependencies;
    // Pathogen upgrade effects
    double infectivityBoost = 0.0;
    double lethalityBoost = 0.0;
    double vaccineDifficultyBoost = 0.0;

    // Humanity upgrade effects
    double awarenessBoost = 0.0;
    double vaccineSpreadRateBoost = 0.0;
    double vaccineEfficacyBoost = 0.0;
};

bool dependenciesSatisfied(const UpgradeDefinition & upgrade,
                           const std::vector<UpgradeId> & purchasedUpgrades);

}
