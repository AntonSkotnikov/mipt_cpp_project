#pragma once

#include "UI_ClientAPI.hpp"

#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

namespace plague {

class GameState {
public:
    GameState() = default;

    GameSituation getSituation() const;
    void setSituation(GameSituation newSituation);

    void resetForMenu();
    void clearNews();

    void setDay(std::uint16_t day);
    void setRole(PlayerRole role);
    void setPlayerInfo(const InfoAboutPlayer& playerInfo);
    void setPlayerPoints(UpgradePointType points);
    void setChoosingSideState(const ChoosingSideState& choosingSide);
    void addNews(ImportanceOfNews importance, const std::string& text);

    GameSnapshot snapshot() const;

private:
    mutable std::mutex mutex_;
    GameSnapshot snapshot_{
        GameSituation::MainMenu,
        0,
        InfoAboutPlayer{PlayerRole::Humanity, 0},
        {}
    };
};

}
