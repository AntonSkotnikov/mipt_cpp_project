#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "GameTypes.hpp"
#include "SimulationTypes.hpp"
#include "Upgrade.hpp"

namespace plague {

struct RoomSummary {
    std::string name;
    bool privateRoom = false;
    std::uint16_t players = 0;
    std::uint16_t capacity = 2;
};

struct GameSnapshot {
    GameSituation situation;
    std::uint16_t day;
    InfoAboutPlayer playerInfo;
    std::vector<RoomSummary> rooms{};
    std::vector<Country> countries{};
    double cureProgress = 0.0;
    std::vector<std::string> news{};
    std::vector<UpgradeDefinition> availableUpgrades{};
    std::vector<UpgradeId> purchasedUpgrades{};

    ChoosingSideState choosingSide{};
};

}
