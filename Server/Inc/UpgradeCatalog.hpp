#pragma once

#include "GameTypes.hpp"
#include "Upgrade.hpp"

#include <vector>

namespace plague {

const std::vector<UpgradeDefinition>& availableUpgradesFor(PlayerRole role);

}  // namespace plague
