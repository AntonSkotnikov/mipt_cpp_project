#include "ScreenShared.hpp"

namespace plague::ui {

ChoosingSideScreen::ChoosingSideScreen(Config & cfg, Window & win) : Screen(cfg, win) {
    auto status = std::make_unique<VariableInfo>(win_, "");
    status_ = status.get();
    auto coloredStatus = std::make_unique<ColorDecorator>(win_, std::move(status), defaultColorPair);
    statusColor_ = coloredStatus.get();
    widgets.push_back(std::move(coloredStatus));

    auto subtypeMenu = std::make_unique<Menu>(win_);
    subtypeMenu_ = subtypeMenu.get();
    for (std::size_t index = 0; index < maxSubtypeCount(); index++) {
        subtypeMenu->addButton("", [index]() -> request::UIRequest {
            return request::ChoosingSide{request::ChoosingSideAction::SelectSubtype, static_cast<int>(index)};
        });
    }
    widgets.push_back(std::make_unique<LabelDecorator>(
        win_,
        std::make_unique<FrameDecorator>(win_, std::move(subtypeMenu)),
        "Subtype"
    ));

    auto description = std::make_unique<Info>(win_, "");
    description_ = description.get();
    widgets.push_back(std::make_unique<LabelDecorator>(
        win_,
        std::make_unique<FrameDecorator>(win_, std::move(description)),
        "Description"
    ));

    widgets.push_back(std::make_unique<FrameDecorator>(
        win_,
        std::make_unique<Button>(win_, "Change side", []() -> request::UIRequest {
            return request::ChoosingSide{request::ChoosingSideAction::ChangeSide, 0};
        })
    ));

    widgets.push_back(std::make_unique<FrameDecorator>(
        win_,
        std::make_unique<Button>(win_, "Ready", []() -> request::UIRequest {
            return request::ChoosingSide{request::ChoosingSideAction::Ready, 0};
        })
    ));

    updateTexts();
    layout();
    focusFirst();
}

void ChoosingSideScreen::updateSnapshot(const GameSnapshot & snapshot) {
    snapshot_ = snapshot;
    updateTexts();
}

void ChoosingSideScreen::updateTexts() {
    const PlayerRole role = snapshot_.playerInfo.role;
    const PlayerSubtype subtype = presentationFor(role, snapshot_.choosingSide.selectedSubtype).subtype;
    const std::size_t count = subtypeCountFor(role);

    if (subtypeMenu_ != nullptr) {
        for (std::size_t i = 0; i < maxSubtypeCount(); i++) {
            const std::string label = i < count ? subtypeAt(role, i).label : "";
            subtypeMenu_->changeButtonText(i, label);
        }
    }

    const SubtypePresentation & selected = presentationFor(role, subtype);
    if (description_ != nullptr) {
        description_->changeText(selected.description);
    }

    if (status_ == nullptr || statusColor_ == nullptr) {
        return;
    }

    int colorPair = defaultColorPair;
    std::string line = std::string("You play as ") + roleName(role) + ": " + selected.label;

    if (snapshot_.choosingSide.opponentSideChangeRequested ||
        snapshot_.choosingSide.signal == ChoosingSideSignal::OpponentRequestsSideChange) {
        line = "Opponent wants to change side";
        colorPair = blueColorPair;
    } else if (snapshot_.choosingSide.opponentReady ||
               snapshot_.choosingSide.signal == ChoosingSideSignal::OpponentReady) {
        line = "Opponent is ready";
        colorPair = blueColorPair;
    } else if (snapshot_.choosingSide.ready ||
               snapshot_.choosingSide.signal == ChoosingSideSignal::LocalReady) {
        line = std::string("Ready: you play as ") + roleName(role) + ": " + selected.label;
        colorPair = greenColorPair;
    }

    status_->changeLine(line);
    statusColor_->setColorPair(colorPair);
}

void ChoosingSideScreen::layout() {
    if (widgets.size() < 5) return;

    const int padding = win_.bordered() ? 2 : 1;
    const int contentX = padding;
    const int contentY = padding;
    const int contentWidth = std::max(1, win_.width() - padding * 2);
    const int contentHeight = std::max(1, win_.height() - padding * 2);
    const int gap = 1;
    const int statusHeight = 1;
    const int bottomHeight = 3;
    const int bodyY = contentY + statusHeight + gap;
    const int bodyHeight = std::max(5, contentHeight - statusHeight - bottomHeight - gap * 2);
    const int bottomY = bodyY + bodyHeight + gap;
    const int menuWidth = std::min(36, std::max(24, contentWidth / 3));
    const int descriptionX = contentX + menuWidth + gap;
    const int descriptionWidth = std::max(1, contentX + contentWidth - descriptionX);

    widgets[choosingStatusIndex]->setRect({contentY, contentX, statusHeight, contentWidth});
    widgets[choosingMenuIndex]->setRect({
        bodyY + 2,
        contentX + 1,
        std::max(1, bodyHeight - 3),
        std::max(1, menuWidth - 2)
    });
    widgets[choosingDescriptionIndex]->setRect({
        bodyY + 2,
        descriptionX + 1,
        std::max(1, bodyHeight - 3),
        std::max(1, descriptionWidth - 2)
    });

    const int buttonWidth = std::min(20, std::max(12, contentWidth / 5));
    widgets[choosingChangeSideIndex]->setRect(innerRect({bottomY, contentX, bottomHeight, buttonWidth}));
    widgets[choosingReadyIndex]->setRect(innerRect({bottomY, contentX + contentWidth - buttonWidth, bottomHeight, buttonWidth}));
}

void ChoosingSideScreen::resize() {
    layout();
}

void ChoosingSideScreen::focusBottomButton(std::size_t index) {
    if (index >= choosingBottomButtonIndices.size()) {
        return;
    }

    focusWidget(choosingBottomButtonIndices[index]);
}

void ChoosingSideScreen::focusNextBottomButton() {
    focusBottomButton(wrappedIndexNear(choosingBottomButtonIndices, focusedIndex_, 1));
}

void ChoosingSideScreen::focusPrevBottomButton() {
    focusBottomButton(wrappedIndexNear(choosingBottomButtonIndices, focusedIndex_, -1));
}

request::UIRequest ChoosingSideScreen::handleInput(int key) {
    const InputResult result = handleFocusedInput(key);
    if (!std::holds_alternative<request::None>(result.request)) {
        return result.request;
    }

    if (result.handled) {
        return request::None{};
    }

    switch (key) {
        case KEY_UP:
            focusWidget(choosingMenuIndex);
            return request::None{};

        case KEY_DOWN:
            focusBottomButton(0);
            return request::None{};

        case KEY_LEFT:
            if (focusedIndex_ == choosingReadyIndex || focusedIndex_ == choosingChangeSideIndex) {
                focusPrevBottomButton();
            } else {
                focusWidget(choosingMenuIndex);
            }
            return request::None{};

        case KEY_RIGHT:
            if (focusedIndex_ == choosingReadyIndex || focusedIndex_ == choosingChangeSideIndex) {
                focusNextBottomButton();
            } else {
                focusBottomButton(1);
            }
            return request::None{};

        case KEY_BTAB:
            focusPrev();
            return request::None{};

        case '\t':
            focusNext();
            return request::None{};
    }

    return request::None{};
}

}  // namespace plague::ui
