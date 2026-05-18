#include "ScreenShared.hpp"

namespace plague::ui {

UpgradeScreen::UpgradeScreen(Config & cfg, Window & win, UpgradeCategory category)
    : Screen(cfg, win), category_(category) {
    for (std::size_t i = 0; i < upgradeTabRequests.size(); i++) {
        widgets.push_back(std::make_unique<FrameDecorator>(
            win_,
            std::make_unique<Button>(win_, upgradeTabLabels[i], [i]() -> request::UIRequest {
                return upgradeTabRequests[i];
            })
        ));
    }

    auto upgradeList = std::make_unique<UpgradeList>(win_);
    upgradeList_ = upgradeList.get();
    upgradeList_->setItems(upgradeItemsFor(snapshot_, category_));
    widgets.push_back(std::make_unique<LabelDecorator>(
        win_,
        std::make_unique<FrameDecorator>(win_, std::move(upgradeList)),
        "Upgrades"
    ));

    auto description = std::make_unique<Info>(win_, "");
    description_ = description.get();
    widgets.push_back(std::make_unique<LabelDecorator>(
        win_,
        std::make_unique<FrameDecorator>(win_, std::move(description)),
        "Description"
    ));

    updateDescription();
    layout();
    focusTab(upgradeTabIndex(category_));
}

void UpgradeScreen::layout() {
    if (widgets.size() <= descriptionIndex_) return;

    const int padding = win_.bordered() ? 2 : 1;
    const int contentX = padding;
    const int contentY = padding;
    const int contentWidth = std::max(1, win_.width() - padding * 2);
    const int contentHeight = std::max(1, win_.height() - padding * 2);
    const int gap = 1;
    const int tabHeight = 3;
    const int tabWidth = std::max(12, (contentWidth - gap * static_cast<int>(tabCount_ - 1)) / static_cast<int>(tabCount_));

    for (std::size_t i = 0; i < tabCount_; i++) {
        const int x = contentX + static_cast<int>(i) * (tabWidth + gap);
        const int width = i == tabCount_ - 1
            ? std::max(1, contentX + contentWidth - x)
            : tabWidth;
        widgets[i]->setRect(innerRect({contentY, x, tabHeight, width}));
    }

    const int bodyY = contentY + tabHeight + gap + 1;
    const int bodyHeight = std::max(1, contentHeight - tabHeight - gap - 2);
    const int listWidth = std::min(42, std::max(28, contentWidth / 3));
    const int descriptionX = contentX + listWidth + gap;
    const int descriptionWidth = std::max(1, contentX + contentWidth - descriptionX);

    widgets[listIndex_]->setRect({
        bodyY + 1,
        contentX,
        std::max(1, bodyHeight - 1),
        std::max(1, listWidth - 1)
    });
    widgets[descriptionIndex_]->setRect({
        bodyY + 1,
        descriptionX,
        std::max(1, bodyHeight - 1),
        descriptionWidth
    });
}

void UpgradeScreen::updateDescription() {
    if (upgradeList_ == nullptr || description_ == nullptr) {
        return;
    }

    const UpgradeListItem * selected = upgradeList_->selectedItem();
    if (selected == nullptr) {
        description_->changeText("No upgrades available");
        return;
    }

    std::string dependencies = "None";
    if (!selected->upgrade.dependencies.empty()) {
        dependencies.clear();
        for (std::size_t i = 0; i < selected->upgrade.dependencies.size(); i++) {
            if (i > 0) {
                dependencies += ", ";
            }
            dependencies += selected->upgrade.dependencies[i];
        }
    }

    std::string status = "Available";
    if (selected->purchased) {
        status = "Purchased";
    } else if (!selected->available) {
        status = "Locked";
    }

    description_->changeText(
        selected->upgrade.title +
        "\n\nCost: " + std::to_string(selected->upgrade.cost) +
        "\nStatus: " + status +
        "\nDependencies: " + dependencies +
        "\n\n" + selected->upgrade.description
    );
}

void UpgradeScreen::focusTab(std::size_t tabIndex) {
    if (tabIndex >= tabCount_) {
        return;
    }

    focusWidget(tabIndex);
}

void UpgradeScreen::focusNextTab() {
    const std::size_t current = focusedIndex_ < tabCount_ ? focusedIndex_ : upgradeTabIndex(category_);
    focusTab((current + 1) % tabCount_);
}

void UpgradeScreen::focusPrevTab() {
    const std::size_t current = focusedIndex_ < tabCount_ ? focusedIndex_ : upgradeTabIndex(category_);
    focusTab(current == 0 ? tabCount_ - 1 : current - 1);
}

request::UIRequest UpgradeScreen::purchaseSelectedUpgrade() const {
    if (upgradeList_ == nullptr) {
        return request::None{};
    }

    const UpgradeListItem * selected = upgradeList_->selectedItem();
    if (selected == nullptr || selected->purchased || !selected->available ||
        selected->upgrade.cost > snapshot_.playerInfo.points) {
        return request::None{};
    }

    return request::PurchaseUpgrade{selected->upgrade.id};
}

void UpgradeScreen::resize() {
    layout();
}

void UpgradeScreen::updateSnapshot(const GameSnapshot & snapshot) {
    snapshot_ = snapshot;
    if (upgradeList_ != nullptr) {
        upgradeList_->setItems(upgradeItemsFor(snapshot_, category_));
    }
    updateDescription();
}

request::UIRequest UpgradeScreen::handleInput(int key) {
    if (key == 27) {
        return request::Game::Back;
    }

    if ((key == KEY_ENTER || key == '\n' || key == '\r') && focusedIndex_ == listIndex_) {
        return purchaseSelectedUpgrade();
    }

    const InputResult result = handleFocusedInput(key);
    if (!std::holds_alternative<request::None>(result.request)) {
        return result.request;
    }

    if (result.handled) {
        updateDescription();
        return request::None{};
    }

    switch (key) {
        case KEY_UP:
            focusWidget(listIndex_);
            updateDescription();
            return request::None{};

        case KEY_DOWN:
            focusWidget(listIndex_);
            updateDescription();
            return request::None{};

        case KEY_LEFT:
            if (focusedIndex_ < tabCount_) {
                focusPrevTab();
            } else {
                focusTab(upgradeTabIndex(category_));
            }
            return request::None{};

        case KEY_RIGHT:
            if (focusedIndex_ < tabCount_) {
                focusNextTab();
            } else {
                focusTab(upgradeTabIndex(category_));
            }
            return request::None{};

        case KEY_BTAB:
            focusPrev();
            updateDescription();
            return request::None{};

        case '\t':
            focusNext();
            updateDescription();
            return request::None{};

        case 't':
        case 'T':
            return request::Game::Transmission;

        case 'c':
        case 'C':
            return request::Game::Clinic;

        case 'a':
        case 'A':
            return request::Game::Abilities;

        case 'u':
        case 'U':
            return upgradeTabRequests[upgradeTabIndex(category_)];
    }

    return request::None{};
}

}  // namespace plague::ui
