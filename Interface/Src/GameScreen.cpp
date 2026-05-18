#include "ScreenShared.hpp"

namespace plague::ui {

GameScreen::GameScreen(Config & cfg, Window & win) : Screen(cfg, win) {
    widgets.push_back(std::make_unique<FrameDecorator>(win_, std::make_unique<Info>(win_, "")));

    for (std::size_t i = 0; i < countryWidgetCount; i++) {
        auto countryImage = std::make_unique<DetalizedImage>(win_);
        countryImages_.push_back(countryImage.get());
        widgets.push_back(std::make_unique<ColorDecorator>(win_, std::move(countryImage), defaultColorPair));
    }

    loadCountryMaps();

    auto pointsInfo = std::make_unique<VariableInfo>(win_, "Points: 0");
    pointsInfo_ = pointsInfo.get();
    widgets.push_back(std::make_unique<FrameDecorator>(win_, std::move(pointsInfo)));

    auto infectedInfo = std::make_unique<VariableInfo>(win_, "Infected: 0");
    infectedInfo_ = infectedInfo.get();
    widgets.push_back(std::make_unique<FrameDecorator>(win_, std::move(infectedInfo)));

    auto selectedCountryInfo = std::make_unique<VariableInfo>(win_, "World overview");
    selectedCountryInfo_ = selectedCountryInfo.get();
    widgets.push_back(std::make_unique<FrameDecorator>(win_, std::move(selectedCountryInfo)));

    auto deadInfo = std::make_unique<VariableInfo>(win_, "Dead: 0");
    deadInfo_ = deadInfo.get();
    widgets.push_back(std::make_unique<FrameDecorator>(win_, std::move(deadInfo)));

    auto cureInfo = std::make_unique<VariableInfo>(win_, "Cure: 0%");
    cureInfo_ = cureInfo.get();
    widgets.push_back(std::make_unique<FrameDecorator>(win_, std::move(cureInfo)));

    widgets.push_back(std::make_unique<FrameDecorator>(win_, std::make_unique<Button>(win_, "Upgrade", []() ->request::UIRequest {
        return request::Game::Upgrade;
    })));
    auto newsTicker = std::make_unique<Ticker>(win_, 6);
    newsTicker_ = newsTicker.get();
    widgets.push_back(std::make_unique<FrameDecorator>(win_, std::move(newsTicker)));

    auto dayInfo = std::make_unique<VariableInfo>(win_, "Day: 0");
    dayInfo_ = dayInfo.get();
    widgets.push_back(std::make_unique<FrameDecorator>(win_, std::move(dayInfo)));

    widgets.push_back(std::make_unique<FrameDecorator>(win_, std::make_unique<Button>(win_, "World", []() ->request::UIRequest {
        return request::Game::World;
    })));

    layout();
    focusCountry(0);
}



void GameScreen::layout() {
    if (widgets.size() < gameStatWidgetStart + 9) return;

    const int padding = win_.bordered() ? 2 : 1;
    const int contentX = padding;
    const int contentY = padding;
    const int contentWidth = std::max(1, win_.width() - padding * 2);
    const int contentHeight = std::max(1, win_.height() - padding * 2);

    const int topHeight = 3;
    const int bottomHeight = 3;
    const int gap = 1;
    const int sideWidth = std::min(34, std::max(24, contentWidth / 4));
    const int mapWidth = std::max(20, contentWidth - sideWidth - gap);
    const int mainY = contentY + topHeight + gap;
    const int mainHeight = std::max(5, contentHeight - topHeight - bottomHeight - gap * 2);
    const int bottomY = mainY + mainHeight + gap;

    const std::size_t topWidgets[] = {gameDnaIndex, gameIllIndex, gameDeadIndex, gameCureIndex};
    const int topItemWidth = std::max(12, (contentWidth - gap * 3) / 4);
    for (std::size_t i = 0; i < 4; i++) {
        const int x = contentX + static_cast<int>(i) * (topItemWidth + gap);
        const int width = (i == 3) ? std::max(1, contentX + contentWidth - x) : topItemWidth;
        widgets[topWidgets[i]]->setRect(innerRect({contentY, x, topHeight, width}));
    }

    const Rect mapOuterRect = {mainY, contentX, mainHeight, mapWidth};
    const Rect mapRect = innerRect(mapOuterRect);
    widgets[mapFrameIndex]->setRect(mapRect);

    for (std::size_t i = 0; i < countryWidgetCount; i++) {
        widgets[countryWidgetStart + i]->setRect(mapRect);
    }

    const int sideX = contentX + mapWidth + gap;
    const int sideItemHeight = 3;
    widgets[gameWorldInfoIndex]->setRect(innerRect({mainY, sideX, sideItemHeight, sideWidth}));
    widgets[gameDayIndex]->setRect(innerRect({mainY + sideItemHeight + gap, sideX, sideItemHeight, sideWidth}));

    const int buttonWidth = std::min(18, std::max(10, sideWidth));
    widgets[gameUpgradeIndex]->setRect(innerRect({bottomY, contentX, bottomHeight, buttonWidth}));
    widgets[gameWorldButtonIndex]->setRect(innerRect({bottomY, contentX + buttonWidth + gap, bottomHeight, buttonWidth}));

    const int tickerX = contentX + buttonWidth * 2 + gap * 2;
    const int tickerWidth = std::max(1, contentX + contentWidth - tickerX);
    widgets[gameTickerIndex]->setRect(innerRect({bottomY, tickerX, bottomHeight, tickerWidth}));
}

void GameScreen::resize() {
    if (!countryMapsLoaded_ || loadedMapResolution_ != cfg_.resolution) {
        loadCountryMaps();
    }

    layout();
}

void GameScreen::updateSnapshot(const GameSnapshot & snapshot) {
    snapshot_ = snapshot;

    if (pointsInfo_ != nullptr) {
        pointsInfo_->changeLine("Points: " + std::to_string(snapshot_.playerInfo.points));
    }
    if (cureInfo_ != nullptr) {
        const int progress = std::clamp(static_cast<int>(snapshot_.cureProgress), 0, 100);
        cureInfo_->changeLine("Cure: " + std::to_string(progress) + "%");
    }
    if (dayInfo_ != nullptr) {
        dayInfo_->changeLine("Day: " + std::to_string(snapshot_.day));
    }

    updateNewsTicker();
    updateCountryHighlights();
    updatePopulationInfo();
    updateSelectedCountryInfo();
}

void GameScreen::loadCountryMaps() {
    if (countryImages_.size() != countryWidgetCount) {
        return;
    }

    countryBounds_.assign(countryWidgetCount, {});

    for (std::size_t i = 0; i < countryWidgetCount; i++) {
        std::vector<SymbolOnScreen> symbols = parseMapCountry(lowMapCountries[i], cfg_.resolution);
        countryBounds_[i] = countryBounds(symbols);

        countryImages_[i]->clearSymbols();
        countryImages_[i]->addSymbols(std::move(symbols));
    }

    updateCountryHighlights();
    loadedMapResolution_ = cfg_.resolution;
    countryMapsLoaded_ = true;
}

void GameScreen::focusCountry(std::size_t countryIndex) {
    if (countryIndex >= countryWidgetCount) {
        return;
    }

    indexOfSelectedCountry = static_cast<int>(countryIndex);
    focusWidget(countryWidgetStart + countryIndex);
    updateSelectedCountryInfo();
}

void GameScreen::focusNearestCountry(int key) {
    if (indexOfSelectedCountry < 0 ||
        static_cast<std::size_t>(indexOfSelectedCountry) >= countryBounds_.size()) {
        focusCountry(0);
        return;
    }

    const std::size_t currentIndex = static_cast<std::size_t>(indexOfSelectedCountry);
    const Rect current = countryBounds_[currentIndex];
    if (!validCountryBounds(current)) {
        return;
    }

    std::size_t bestIndex = currentIndex;
    int bestScore = std::numeric_limits<int>::max();

    for (std::size_t i = 0; i < countryBounds_.size(); i++) {
        if (i == currentIndex || !validCountryBounds(countryBounds_[i])) {
            continue;
        }

        const Rect candidate = countryBounds_[i];
        if (!isCountryInDirection(current, candidate, key)) {
            continue;
        }

        const int score = directionalScore(current, candidate, key);
        if (score < bestScore) {
            bestScore = score;
            bestIndex = i;
        }
    }

    if (bestIndex != currentIndex) {
        focusCountry(bestIndex);
    }
}

void GameScreen::focusActionButton(std::size_t buttonIndex) {
    if (buttonIndex >= gameActionButtonIndices.size()) {
        return;
    }

    navigatingCountries_ = false;
    focusWidget(gameActionButtonIndices[buttonIndex]);
}

void GameScreen::focusNextActionButton() {
    focusActionButton(wrappedIndexNear(gameActionButtonIndices, focusedIndex_, 1));
}

void GameScreen::focusPrevActionButton() {
    focusActionButton(wrappedIndexNear(gameActionButtonIndices, focusedIndex_, -1));
}

void GameScreen::toggleNavigationMode() {
    if (navigatingCountries_) {
        focusActionButton(0);
        return;
    }

    navigatingCountries_ = true;
    focusCountry(indexOfSelectedCountry < 0 ? 0 : static_cast<std::size_t>(indexOfSelectedCountry));
}

void GameScreen::updateNewsTicker() {
    if (newsTicker_ == nullptr) {
        return;
    }

    if (snapshot_.news.size() < displayedNewsCount_) {
        displayedNewsCount_ = 0;
    }

    for (std::size_t i = displayedNewsCount_; i < snapshot_.news.size(); i++) {
        newsTicker_->addLine(snapshot_.news[i]);
    }

    displayedNewsCount_ = snapshot_.news.size();
}

void GameScreen::updateCountryHighlights() {
    for (std::size_t i = 0; i < countryImages_.size(); i++) {
        const bool highlighted = i < snapshot_.highlightedCountries.size() && snapshot_.highlightedCountries[i];
        countryImages_[i]->setEventHighlight(highlighted);
    }
}

void GameScreen::updatePopulationInfo() {
    if (infectedInfo_ == nullptr || deadInfo_ == nullptr) {
        return;
    }

    const bool hasSelectedCountry =
        indexOfSelectedCountry >= 0 &&
        static_cast<std::size_t>(indexOfSelectedCountry) < lowMapCountries.size();

    if (hasSelectedCountry) {
        const std::string_view countryName = lowMapCountries[static_cast<std::size_t>(indexOfSelectedCountry)];
        if (const Country * country = findCountry(snapshot_.countries, countryName)) {
            infectedInfo_->changeLine("Infected: " + formatCount(countryInfectedCount(*country)));
            deadInfo_->changeLine("Dead: " + formatCount(countryDeadCount(*country)));
            return;
        }
    }

    infectedInfo_->changeLine("Infected: " + formatCount(totalInfectedCount(snapshot_.countries)));
    deadInfo_->changeLine("Dead: " + formatCount(totalDeadCount(snapshot_.countries)));
}

void GameScreen::updateSelectedCountryInfo() {
    if (selectedCountryInfo_ == nullptr) {
        return;
    }

    if (indexOfSelectedCountry < 0) {
        selectedCountryInfo_->changeLine("World overview");
        updatePopulationInfo();
        return;
    }

    const std::string_view countryName = lowMapCountries[static_cast<std::size_t>(indexOfSelectedCountry)];
    selectedCountryInfo_->changeLine(std::string("Country: ") + std::string(countryName));
    updatePopulationInfo();
}

request::UIRequest GameScreen::handleInput(int key) {
    const InputResult result = handleFocusedInput(key);
    if (!std::holds_alternative<request::None>(result.request)) {
        return result.request;
    }

    if (result.handled) {
        return request::None{};
    }

    switch (key) {
        case KEY_LEFT:
        case KEY_UP:
            if (navigatingCountries_) {
                focusNearestCountry(key);
            } else {
                focusPrevActionButton();
            }
            return request::None{};

        case KEY_RIGHT:
        case KEY_DOWN:
            if (navigatingCountries_) {
                focusNearestCountry(key);
            } else {
                focusNextActionButton();
            }
            return request::None{};

        case KEY_BTAB:
        case '\t':
            toggleNavigationMode();
            return request::None{};

        case KEY_ENTER:
        case '\n':
        case '\r':
            if (navigatingCountries_ &&
                indexOfSelectedCountry >= 0 &&
                static_cast<std::size_t>(indexOfSelectedCountry) < lowMapCountries.size()) {
                return request::SelectCountry{
                    lowMapCountries[static_cast<std::size_t>(indexOfSelectedCountry)]
                };
            }
            return request::None{};

        case 'u':
        case 'U':
            return request::Game::Upgrade;

        case 'i':
        case 'I':
            return request::Game::Info;

        case 't':
        case 'T':
            return request::Game::Transmission;

        case 'c':
        case 'C':
            return request::Game::Clinic;

        case 'a':
        case 'A':
            return request::Game::Abilities;

        case 'w':
        case 'W':
            return request::Game::World;

        case 'r':
        case 'R':
            return request::Game::Cure;

        case 'n':
        case 'N':
            return request::Game::News;
    }

    return request::None{};
}

}  // namespace plague::ui
