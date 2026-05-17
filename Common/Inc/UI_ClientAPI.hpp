#pragma once

#include <cstdint>
#include <vector>

#include "GameTypes.hpp"
#include "SimulationTypes.hpp"

namespace plague {

struct GameSnapshot {
    GameSituation situation;
    std::uint16_t day;
    InfoAboutPlayer playerInfo;
    std::vector<Country> countries{};
    double cureProgress = 0.0;

    ChoosingSideState choosingSide{};
};

}
