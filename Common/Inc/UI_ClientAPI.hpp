#pragma once

#include <cstdint>

#include "GameTypes.hpp"

namespace plague {



struct GameSnapshot {
    GameSituation situation;
    std::uint16_t day;
    InfoAboutPlayer playerInfo;

    ChoosingSideState choosingSide{};
};

}
