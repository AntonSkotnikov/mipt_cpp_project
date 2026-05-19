#include "ScreenShared.hpp"

#include <sstream>

namespace plague::ui {

namespace {

bool humanityWon(const GameSnapshot & snapshot) {
    if (snapshot.cureProgress >= 100.0) {
        return true;
    }

    if (snapshot.countries.empty()) {
        return false;
    }

    double living = 0.0;
    double infected = 0.0;
    for (const Country & country : snapshot.countries) {
        living += country.pop.susceptible + country.pop.exposed + country.pop.infected + country.pop.recovered;
        infected += country.pop.exposed + country.pop.infected;
    }

    return living > 0.0 && infected <= 0.0;
}

const char * roleTitle(PlayerRole role) {
    return role == PlayerRole::Humanity ? "Humanity" : "Pathogen";
}

}  // namespace

EndScreen::EndScreen(Config & cfg, Window & win) : Screen(cfg, win) {
    auto result = std::make_unique<Info>(win_, "");
    resultInfo_ = result.get();
    widgets.push_back(std::make_unique<FrameDecorator>(win_, std::move(result)));

    widgets.push_back(std::make_unique<FrameDecorator>(
        win_,
        std::make_unique<Button>(win_, "Back to menu", []() -> request::UIRequest {
            return request::Settings::Back;
        })
    ));

    updateText();
    layout();
    focusFirst();
}

void EndScreen::updateSnapshot(const GameSnapshot & snapshot) {
    snapshot_ = snapshot;
    updateText();
}

void EndScreen::updateText() {
    if (resultInfo_ == nullptr) {
        return;
    }

    const bool humanityVictory = humanityWon(snapshot_);
    const bool playerVictory =
        (snapshot_.playerInfo.role == PlayerRole::Humanity && humanityVictory) ||
        (snapshot_.playerInfo.role == PlayerRole::Pathogen && !humanityVictory);

    std::ostringstream out;
    out << (playerVictory ? "Victory" : "Defeat") << "\n\n";
    out << "Winner: " << (humanityVictory ? "Humanity" : "Pathogen") << "\n";
    out << "Your side: " << roleTitle(snapshot_.playerInfo.role) << "\n";
    out << "Day: " << snapshot_.day << "\n";
    out << "Cure progress: " << static_cast<int>(snapshot_.cureProgress) << "%\n\n";

    if (humanityVictory) {
        out << "The outbreak has been contained. Humanity survives.";
    } else {
        out << "The pathogen overwhelmed the world. Humanity has fallen.";
    }

    resultInfo_->changeText(out.str());
}

void EndScreen::layout() {
    if (widgets.size() < 2) return;

    const int padding = win_.bordered() ? 2 : 1;
    const int contentWidth = std::max(1, win_.width() - padding * 2);
    const int contentHeight = std::max(1, win_.height() - padding * 2);
    const int panelWidth = std::min(72, std::max(40, contentWidth / 2));
    const int panelHeight = std::min(14, std::max(8, contentHeight - 8));
    const int panelX = padding + std::max(0, (contentWidth - panelWidth) / 2);
    const int panelY = padding + std::max(0, (contentHeight - panelHeight - 4) / 2);
    const int buttonWidth = std::min(24, std::max(14, panelWidth / 3));
    const int buttonX = padding + std::max(0, (contentWidth - buttonWidth) / 2);
    const int buttonY = panelY + panelHeight + 2;

    widgets[0]->setRect({panelY + 1, panelX + 1, panelHeight - 2, panelWidth - 2});
    widgets[1]->setRect({buttonY + 1, buttonX + 1, 1, buttonWidth - 2});
}

void EndScreen::resize() {
    layout();
}

request::UIRequest EndScreen::handleInput(int key) {
    const InputResult result = handleFocusedInput(key);
    if (!std::holds_alternative<request::None>(result.request)) {
        return result.request;
    }

    return request::None{};
}

}  // namespace plague::ui
