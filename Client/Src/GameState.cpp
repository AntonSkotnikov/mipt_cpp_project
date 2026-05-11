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
    snapshot_.recentNews.clear();
    snapshot_.choosingSide = ChoosingSideState{};
}

void GameState::clearNews() {
    std::lock_guard<std::mutex> lock(mutex_);
    snapshot_.recentNews.clear();
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

void GameState::addNews(ImportanceOfNews importance, const std::string& text) {
    if (text.empty()) {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    snapshot_.recentNews.emplace_back(importance, text);
}

GameSnapshot GameState::snapshot() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return snapshot_;
}

}
