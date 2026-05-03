#pragma once

#include <cstdint>
#include <vector>

#include "GameTypes.hpp"

namespace plague {



struct GameSnapshot {
    GameSituation situation;
    std::uint16_t day;
    InfoAboutPlayer playerInfo;

    std::vector<News> recentNews;
};

}