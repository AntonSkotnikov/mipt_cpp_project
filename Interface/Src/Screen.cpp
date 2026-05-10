#include "MapParser.hpp"
#include "Screen.hpp"
#include "Settings.hpp"
#include "UIRequest.hpp"
#include "Widget.hpp"
#include "Window.hpp"
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdlib>
#include <iterator>
#include <limits>
#include <memory>
#include <ncurses.h>
#include <variant>


namespace plague::ui {

namespace {

constexpr std::array<const char *, 29> lowMapCountries = {
    "AUSTRALIA",
    "BELARUS",
    "BRAZIL",
    "CANADA",
    "CHINA",
    "EAST",
    "GREENLAND",
    "ICELAND",
    "INDIA",
    "JAPAN",
    "KAZAKHSTAN",
    "M AFRICA",
    "MADAGASCAR",
    "MEXICO",
    "MIDDLE EAST",
    "MONGOLIA",
    "N AFRICA",
    "N SOUTH AMERICA",
    "NEW ZELAND",
    "OCEANIA",
    "RUSSIA",
    "S AFRICA",
    "SCANDINAVIA",
    "SW SOUTH AMERICA",
    "TURKEY",
    "UK",
    "UKRAINE",
    "USA",
    "W EUROPE"
};

constexpr std::size_t countryWidgetCount = lowMapCountries.size();
constexpr std::size_t mapFrameIndex = 0;
constexpr std::size_t countryWidgetStart = mapFrameIndex + 1;
constexpr std::size_t gameStatWidgetStart = countryWidgetStart + countryWidgetCount;
constexpr std::size_t gameDnaIndex = gameStatWidgetStart;
constexpr std::size_t gameIllIndex = gameStatWidgetStart + 1;
constexpr std::size_t gameWorldInfoIndex = gameStatWidgetStart + 2;
constexpr std::size_t gameDeadIndex = gameStatWidgetStart + 3;
constexpr std::size_t gameCureIndex = gameStatWidgetStart + 4;
constexpr std::size_t gameUpgradeIndex = gameStatWidgetStart + 5;
constexpr std::size_t gameTickerIndex = gameStatWidgetStart + 6;
constexpr std::size_t gameDayIndex = gameStatWidgetStart + 7;
constexpr std::size_t gameWorldButtonIndex = gameStatWidgetStart + 8;
constexpr std::array<std::size_t, 2> gameActionButtonIndices = {
    gameUpgradeIndex,
    gameWorldButtonIndex
};

struct PanelLayout {
    Rect buttons;
    Rect body;
};

Rect innerRect(Rect outer) {
    return {
        outer.y + 1,
        outer.x + 1,
        std::max(1, outer.height - 2),
        std::max(1, outer.width - 2)
    };
}

Rect countryBounds(const std::vector<SymbolOnScreen> & symbols) {
    if (symbols.empty()) {
        return {};
    }

    int minY = symbols.front().y;
    int maxY = symbols.front().y;
    int minX = symbols.front().x;
    int maxX = symbols.front().x;

    for (const SymbolOnScreen & symbol : symbols) {
        minY = std::min(minY, symbol.y);
        maxY = std::max(maxY, symbol.y);
        minX = std::min(minX, symbol.x);
        maxX = std::max(maxX, symbol.x);
    }

    return {
        minY,
        minX,
        maxY - minY + 1,
        maxX - minX + 1
    };
}

bool validCountryBounds(Rect bounds) {
    return bounds.height > 0 && bounds.width > 0;
}

int centerY(Rect bounds) {
    return bounds.y + bounds.height / 2;
}

int centerX(Rect bounds) {
    return bounds.x + bounds.width / 2;
}

bool isCountryInDirection(Rect current,
                          Rect candidate,
                          int key) {
    switch (key) {
        case KEY_LEFT:
            return centerX(candidate) < centerX(current);
        case KEY_RIGHT:
            return centerX(candidate) > centerX(current);
        case KEY_UP:
            return centerY(candidate) < centerY(current);
        case KEY_DOWN:
            return centerY(candidate) > centerY(current);
    }

    return false;
}

int directionalScore(Rect current,
                     Rect candidate,
                     int key) {
    const int dx = centerX(candidate) - centerX(current);
    const int dy = centerY(candidate) - centerY(current);
    const int primary = (key == KEY_LEFT || key == KEY_RIGHT) ? std::abs(dx) : std::abs(dy);
    const int secondary = (key == KEY_LEFT || key == KEY_RIGHT) ? std::abs(dy) : std::abs(dx);

    return primary * primary + secondary * secondary * 4;
}
/*
PanelLayout gamePanelLayout(Window & win) {
    const int padding = win.bordered() ? 2 : 1;
    const int gap = 1;
    const int contentX = padding;
    const int contentY = padding;
    const int contentWidth = std::max(1, win.width() - padding * 2);
    const int contentHeight = std::max(1, win.height() - padding * 2);
    const int buttonsHeight = 3;
    const int bodyY = contentY + buttonsHeight + gap;
    const int bodyHeight = std::max(3, contentHeight - buttonsHeight - gap);

    return {
        {contentY, contentX, buttonsHeight, contentWidth},
        innerRect({bodyY, contentX, bodyHeight, contentWidth})
    };
}
*/
}

Screen::Screen(Config & cfg, Window & mainWin) : cfg_(cfg), win_(mainWin) {};

int Screen::getKey() {
    return win_.getKey();
}

void Screen::resize() {}

void Screen::focusFirst() {
    for (std::size_t i = 0; i < widgets.size(); i++) {
        if (widgets[i]->focusable()) {
            focusWidget(i);
            return;
        }
    }
}

void Screen::focusWidget(std::size_t index) {
    if (index >= widgets.size() || !widgets[index]->focusable()) return;

    if (focusedIndex_ < widgets.size()) {
        widgets[focusedIndex_]->setFocus(false);
    }

    focusedIndex_ = index;
    widgets[focusedIndex_]->setFocus(true);
}

void Screen::focusNext() {
    if (widgets.empty()) return;

    for (std::size_t step = 1; step <= widgets.size(); step++) {
        const std::size_t index = (focusedIndex_ + step) % widgets.size();
        if (widgets[index]->focusable()) {
            focusWidget(index);
            return;
        }
    }
}

void Screen::focusPrev() {
    if (widgets.empty()) return;

    for (std::size_t step = 1; step <= widgets.size(); step++) {
        const std::size_t index = (focusedIndex_ + widgets.size() - step) % widgets.size();
        if (widgets[index]->focusable()) {
            focusWidget(index);
            return;
        }
    }
}

void Screen::draw() {
    win_.clear();

    for (auto & widget : widgets) {
        widget->draw();
    }

    win_.refresh();
}

Widget * Screen::focusedWidget() {
    if (focusedIndex_ >= widgets.size()) return nullptr;
    return widgets[focusedIndex_].get();
}


MainMenuScreen::MainMenuScreen(Config & cfg, Window & win) : Screen(cfg, win) {
    auto logoWidget = std::make_unique<Info>(win_, R"(
                ______ _                          _____             
                | ___ \ |                        |_   _|            
                | |_/ / | __ _  __ _ _   _  ___    | | _ __   ___   
                |  __/| |/ _` |/ _` | | | |/ _ \   | || '_ \ / __|  
                | |   | | (_| | (_| | |_| |  __/  _| || | | | (__ _ 
                \_|   |_|\__,_|\__, |\__,_|\___|  \___/_| |_|\___(_)
                                __/ |                               
                               |___/                                )");
            
    auto menuWidget = std::make_unique<Menu>(win_);

    menuWidget->addButton("Connect to server", []() -> request::UIRequest {
        return request::MainMenu::ConnectToServer;
    });

    menuWidget->addButton("Settings", []() -> request::UIRequest {
        return request::MainMenu::OpenSettings;
    });

    menuWidget->addButton("Quit", []() -> request::UIRequest {
        return request::MainMenu::Exit;
    });

    widgets.push_back(std::move(logoWidget));
    widgets.push_back(std::move(menuWidget));

    layout();

    focusFirst();
} 

void MainMenuScreen::layout() {
    const int padding = win_.bordered() ? 2 : 1;
    const int contentWidth = std::max(1, win_.width() - padding * 2);
    const int logoHeight = 9;
    const int buttonY = cfg_.resolution == Resolutions::Low ? 28 : (cfg_.resolution == Resolutions::Medium ? 30 : 32);
    const int buttonWidth = 20;

    if (widgets.size() < 2) return;

    widgets[0]->setRect({padding, padding, logoHeight, contentWidth});
    widgets[1]->setRect({buttonY, padding, 3, buttonWidth});
}

void MainMenuScreen::resize() {
    layout();
}

request::UIRequest MainMenuScreen::handleInput(int key) {
    if (Widget * widget = focusedWidget()) {
        const InputResult result = widget->handleInput(key);
        if (!std::holds_alternative<request::None>(result.request)) {
            return result.request;
        }

        if (result.handled) {
            return request::None{};
        }
    }

    switch (key) {
        case KEY_LEFT:
        case KEY_BTAB:
            focusPrev();
            return request::None{};

        case KEY_RIGHT:
        case '\t':
            focusNext();
            return request::None{};
    }

    return request::None{};
}

SmallTermScreen::SmallTermScreen(Config & cfg, Window & win) : Screen(cfg, win) {
    widgets.push_back(std::make_unique<FrameDecorator>(
        win_,
        std::make_unique<Dialog>(win_, "Terminal is too small\nPlease resize it")
    ));
    layout();
}

void SmallTermScreen::layout() {
    if (widgets.empty()) return;

    if (win_.height() <= 2 || win_.width() <= 2) {
        widgets[0]->setRect({0, 0, win_.height(), win_.width()});
        return;
    }

    widgets[0]->setRect({1, 1, win_.height() - 2, win_.width() - 2});
}

void SmallTermScreen::resize() {
    layout();
}

request::UIRequest SmallTermScreen::handleInput(int key) {
    (void)key;
    return request::None{};
}

ConnectToServerScreen::ConnectToServerScreen(Config & cfg, Window & win) : Screen(cfg, win) {
    auto addressInput = std::make_unique<TextInput>(win_);
    auto portInput = std::make_unique<TextInput>(win_);

    TextInput * address = addressInput.get();
    TextInput * port = portInput.get();

    widgets.push_back(std::make_unique<LabelDecorator>(
        win_,
        std::make_unique<FrameDecorator>(win_, std::move(addressInput)),
        "Address"
    ));

    widgets.push_back(std::make_unique<LabelDecorator>(
        win_,
        std::make_unique<FrameDecorator>(win_, std::move(portInput)),
        "Port"
    ));

    auto menuWidget = std::make_unique<Menu>(win_);
    menuWidget->addButton("Connect", [address, port]() -> request::UIRequest {
        return request::ConnectInfo{
            request::Connect::Connect,
            address->getText(),
            port->getText()
        };
    });

    menuWidget->addButton("Back", []() -> request::UIRequest {
        return request::ConnectInfo{request::Connect::Back, "", ""};
    });

    widgets.push_back(std::move(menuWidget));

    layout();
    focusFirst();
}


void ConnectToServerScreen::layout() {
    if (widgets.size() < 2) return;

    const int padding = win_.bordered() ? 2 : 1;
    const int contentWidth = std::max(1, win_.width() - padding * 2);
    const int fieldWidth = std::min(46, std::max(1, contentWidth - 2));
    const int x = padding;

    widgets[0]->setRect({padding + 2, x + 1, 1, fieldWidth});
    widgets[1]->setRect({padding + 7, x + 1, 1, fieldWidth});
    widgets[2]->setRect({padding + 11, x, 2, std::min(20, fieldWidth)});
}

void ConnectToServerScreen::resize() {
    layout();
}

request::UIRequest ConnectToServerScreen::handleInput(int key) {
    if (Widget * widget = focusedWidget()) {
        const InputResult result = widget->handleInput(key);
        if (!std::holds_alternative<request::None>(result.request)) {
            return result.request;
        }

        if (result.handled) {
            return request::None{};
        }
    }

    switch (key) {
        case KEY_UP:
        case KEY_BTAB:
            focusPrev();
            return request::None{};

        case KEY_DOWN:
        case '\t':
            focusNext();
            return request::None{};
    }

    return request::None{};
}

GameScreen::GameScreen(Config & cfg, Window & win) : Screen(cfg, win) {
    widgets.push_back(std::make_unique<FrameDecorator>(win_, std::make_unique<Info>(win_, "")));

    for (std::size_t i = 0; i < countryWidgetCount; i++) {
        auto countryImage = std::make_unique<DetalizedImage>(win_);
        countryImages_.push_back(countryImage.get());
        widgets.push_back(std::make_unique<ColorDecorator>(win_, std::move(countryImage), COLOR_BLACK));
    }

    loadCountryMaps();

    widgets.push_back(std::make_unique<FrameDecorator>(win_, std::make_unique<VariableInfo>(win_, "DNA: 0")));
    widgets.push_back(std::make_unique<FrameDecorator>(win_, std::make_unique<VariableInfo>(win_, "Ill: 0")));
    auto selectedCountryInfo = std::make_unique<VariableInfo>(win_, "World overview");
    selectedCountryInfo_ = selectedCountryInfo.get();
    widgets.push_back(std::make_unique<FrameDecorator>(win_, std::move(selectedCountryInfo)));
    widgets.push_back(std::make_unique<FrameDecorator>(win_, std::make_unique<VariableInfo>(win_, "Dead: 0")));
    widgets.push_back(std::make_unique<FrameDecorator>(win_, std::make_unique<VariableInfo>(win_, "Cure: 0%")));

    widgets.push_back(std::make_unique<FrameDecorator>(win_, std::make_unique<Button>(win_, "Upgrade", []() ->request::UIRequest {
        return request::Game::Upgrade;
    })));
    auto newsTicker = std::make_unique<Ticker>(win_, 6);
    widgets.push_back(std::make_unique<FrameDecorator>(win_, std::move(newsTicker)));
    widgets.push_back(std::make_unique<FrameDecorator>(win_, std::make_unique<VariableInfo>(win_, "Day: 0")));
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

void GameScreen::focusNextCountry() {
    const std::size_t current = indexOfSelectedCountry < 0
        ? 0
        : static_cast<std::size_t>(indexOfSelectedCountry);
    focusCountry((current + 1) % countryWidgetCount);
}

void GameScreen::focusPrevCountry() {
    const std::size_t current = indexOfSelectedCountry < 0
        ? 0
        : static_cast<std::size_t>(indexOfSelectedCountry);
    focusCountry((current + countryWidgetCount - 1) % countryWidgetCount);
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
    const auto it = std::find(gameActionButtonIndices.begin(), gameActionButtonIndices.end(), focusedIndex_);
    const std::size_t current = it == gameActionButtonIndices.end()
        ? 0
        : static_cast<std::size_t>(std::distance(gameActionButtonIndices.begin(), it));

    focusActionButton((current + 1) % gameActionButtonIndices.size());
}

void GameScreen::focusPrevActionButton() {
    const auto it = std::find(gameActionButtonIndices.begin(), gameActionButtonIndices.end(), focusedIndex_);
    const std::size_t current = it == gameActionButtonIndices.end()
        ? 0
        : static_cast<std::size_t>(std::distance(gameActionButtonIndices.begin(), it));

    focusActionButton((current + gameActionButtonIndices.size() - 1) % gameActionButtonIndices.size());
}

void GameScreen::toggleNavigationMode() {
    if (navigatingCountries_) {
        focusActionButton(0);
        return;
    }

    navigatingCountries_ = true;
    focusCountry(indexOfSelectedCountry < 0 ? 0 : static_cast<std::size_t>(indexOfSelectedCountry));
}

bool GameScreen::focusedOnCountry() const {
    return focusedIndex_ >= countryWidgetStart &&
           focusedIndex_ < countryWidgetStart + countryWidgetCount;
}

void GameScreen::updateSelectedCountryInfo() {
    if (selectedCountryInfo_ == nullptr) {
        return;
    }

    if (indexOfSelectedCountry < 0) {
        selectedCountryInfo_->changeLine("World overview");
        return;
    }

    selectedCountryInfo_->changeLine(
        std::string("Country: ") + lowMapCountries[static_cast<std::size_t>(indexOfSelectedCountry)]
    );
}

request::UIRequest GameScreen::handleInput(int key) {
    if (Widget * widget = focusedWidget()) {
        const InputResult result = widget->handleInput(key);
        if (!std::holds_alternative<request::None>(result.request)) {
            return result.request;
        }

        if (result.handled) {
            return request::None{};
        }
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



}
