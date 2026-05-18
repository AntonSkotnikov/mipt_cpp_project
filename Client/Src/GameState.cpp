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
    snapshot_.rooms.clear();
    snapshot_.countries.clear();
    snapshot_.availableUpgrades.clear();
    snapshot_.purchasedUpgrades.clear();
    snapshot_.cureProgress = 0.0;
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

void GameState::setCountries(const std::vector<Country>& countries) {
    std::lock_guard<std::mutex> lock(mutex_);
    snapshot_.countries = countries;
}

void GameState::setRooms(const std::vector<RoomSummary>& rooms) {
    std::lock_guard<std::mutex> lock(mutex_);
    snapshot_.rooms = rooms;
}

void GameState::setAvailableUpgrades(const std::vector<UpgradeDefinition>& upgrades) {
    std::lock_guard<std::mutex> lock(mutex_);
    snapshot_.availableUpgrades = upgrades;
}

void GameState::setCureProgress(double cureProgress) {
    std::lock_guard<std::mutex> lock(mutex_);
    snapshot_.cureProgress = cureProgress;
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
