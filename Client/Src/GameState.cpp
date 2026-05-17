#include "GameState.hpp"

namespace plague {

GameSituation GameState::getSituation() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return snapshot_.situation;
}

void GameState::setSituation(GameSituation newSituation) {
    std::lock_guard<std::mutex> lock(mutex_);
    snapshot_.situation = newSituation;
}

void GameState::resetForMenu() {
    std::lock_guard<std::mutex> lock(mutex_);
    snapshot_.day = 0;
    snapshot_.playerInfo.points = 0;
    snapshot_.choosingSide = ChoosingSideState{};
}

void GameState::setDay(std::uint16_t day) {
    std::lock_guard<std::mutex> lock(mutex_);
    snapshot_.day = day;
}

void GameState::setRole(PlayerRole role) {
    std::lock_guard<std::mutex> lock(mutex_);
    snapshot_.playerInfo.role = role;
    snapshot_.playerInfo.points = 100;
    snapshot_.day = 1;
}

void GameState::setPlayerInfo(const InfoAboutPlayer& playerInfo) {
    std::lock_guard<std::mutex> lock(mutex_);
    snapshot_.playerInfo = playerInfo;
}

void GameState::setPlayerPoints(UpgradePointType points) {
    std::lock_guard<std::mutex> lock(mutex_);
    snapshot_.playerInfo.points = points;
}

void GameState::setChoosingSideState(const ChoosingSideState& choosingSide) {
    std::lock_guard<std::mutex> lock(mutex_);
    snapshot_.choosingSide = choosingSide;
}

GameSnapshot GameState::snapshot() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return snapshot_;
}

}
