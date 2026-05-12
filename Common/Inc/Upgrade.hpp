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
};

bool dependenciesSatisfied(const UpgradeDefinition & upgrade,
                           const std::vector<UpgradeId> & purchasedUpgrades);

}
