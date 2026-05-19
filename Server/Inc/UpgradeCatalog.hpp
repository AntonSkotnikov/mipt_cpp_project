#pragma once

#include "GameTypes.hpp"
#include "Upgrade.hpp"

#include <vector>

namespace plague {

// Единый каталог апгрейдов для обеих ролей.
const std::vector<UpgradeDefinition>& availableUpgradesFor(PlayerRole role);

}  // namespace plague
