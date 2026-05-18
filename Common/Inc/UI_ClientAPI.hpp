#pragma once

#include <cstdint>
#include <vector>

#include "GameTypes.hpp"
#include "SimulationTypes.hpp"
#include "Upgrade.hpp"

namespace plague {

struct GameSnapshot {
    GameSituation situation;
    std::uint16_t day;
    InfoAboutPlayer playerInfo;
    std::vector<Country> countries{};
    double cureProgress = 0.0;
    std::vector<UpgradeDefinition> availableUpgrades{};
    std::vector<UpgradeId> purchasedUpgrades{};

    ChoosingSideState choosingSide{};
};

}
