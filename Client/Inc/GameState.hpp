#pragma once

#include "UI_ClientAPI.hpp"

#include <cstdint>
#include <mutex>
#include <vector>

namespace plague {

// Потокобезопасное хранилище текущего снимка игры для UI и сетевого слоя
class GameState {
public:
    GameState() = default;

    GameSituation getSituation() const;
    void setSituation(GameSituation newSituation);

    void resetForMenu();

    void setDay(std::uint16_t day);
    void setRole(PlayerRole role);
    void setPlayerInfo(const InfoAboutPlayer& playerInfo);
    void setPlayerPoints(UpgradePointType points);
    void setCountries(const std::vector<Country>& countries);
    void setHighlightedCountries(const std::vector<bool>& highlightedCountries);
    void setNews(const std::vector<std::string>& news);
    void setRooms(const std::vector<RoomSummary>& rooms);
    void setAvailableUpgrades(const std::vector<UpgradeDefinition>& upgrades);
    void setPurchasedUpgrades(const std::vector<UpgradeId>& upgrades);
    void setCureProgress(double cureProgress);
    void setChoosingSideState(const ChoosingSideState& choosingSide);

    GameSnapshot snapshot() const;

private:
    mutable std::mutex mutex_;
    GameSnapshot snapshot_{
        GameSituation::MainMenu,
        0,
        InfoAboutPlayer{PlayerRole::Humanity, 0}
    };
};

}
