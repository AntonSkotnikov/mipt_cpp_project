#include "ScreenShared.hpp"

namespace plague::ui {

namespace {

std::vector<RoomListItem> roomItemsFromSnapshot(const GameSnapshot & snapshot) {
    std::vector<RoomListItem> items;
    items.reserve(snapshot.rooms.size());

    for (const RoomSummary & room : snapshot.rooms) {
        items.push_back({
            room.name,
            room.privateRoom,
            room.players,
            room.capacity
        });
    }

    return items;
}

}  // namespace

RoomBrowserScreen::RoomBrowserScreen(Config & cfg, Window & win) : Screen(cfg, win) {
    auto roomList = std::make_unique<RoomList>(win_);
    roomList_ = roomList.get();
    widgets.push_back(std::make_unique<LabelDecorator>(
        win_,
        std::make_unique<FrameDecorator>(win_, std::move(roomList)),
        "Rooms"
    ));

    auto selectedPassword = std::make_unique<TextInput>(win_);
    selectedPassword_ = selectedPassword.get();
    widgets.push_back(std::make_unique<LabelDecorator>(
        win_,
        std::make_unique<FrameDecorator>(win_, std::move(selectedPassword)),
        "Room password"
    ));

    auto createName = std::make_unique<TextInput>(win_);
    createName_ = createName.get();
    widgets.push_back(std::make_unique<LabelDecorator>(
        win_,
        std::make_unique<FrameDecorator>(win_, std::move(createName)),
        "New room name"
    ));

    auto createPassword = std::make_unique<TextInput>(win_);
    createPassword_ = createPassword.get();
    widgets.push_back(std::make_unique<LabelDecorator>(
        win_,
        std::make_unique<FrameDecorator>(win_, std::move(createPassword)),
        "New room password"
    ));

    auto status = std::make_unique<VariableInfo>(win_, "");
    status_ = status.get();
    widgets.push_back(std::move(status));

    widgets.push_back(std::make_unique<FrameDecorator>(
        win_,
        std::make_unique<Button>(win_, "Join", [this]() -> request::UIRequest {
            return joinSelectedRoom();
        })
    ));

    widgets.push_back(std::make_unique<FrameDecorator>(
        win_,
        std::make_unique<Button>(win_, "Create", [this]() -> request::UIRequest {
            return createRoom();
        })
    ));

    widgets.push_back(std::make_unique<FrameDecorator>(
        win_,
        std::make_unique<Button>(win_, "Back", []() -> request::UIRequest {
            return request::RoomRequest{request::RoomAction::Back, "", ""};
        })
    ));

    updateRooms();
    layout();
    focusFirst();
}

void RoomBrowserScreen::updateSnapshot(const GameSnapshot & snapshot) {
    snapshot_ = snapshot;
    updateRooms();
}

void RoomBrowserScreen::updateRooms() {
    if (roomList_ != nullptr) {
        roomList_->setItems(roomItemsFromSnapshot(snapshot_));
    }
    updateStatus();
}

void RoomBrowserScreen::updateStatus() {
    if (status_ == nullptr) {
        return;
    }

    std::string line = std::to_string(snapshot_.rooms.size()) + " rooms";
    if (const RoomListItem * selected = roomList_ == nullptr ? nullptr : roomList_->selectedItem()) {
        line += " | selected: " + selected->name;
        line += selected->privateRoom ? " | password required" : " | public";
    } else {
        line += " | create a room or wait for the server list";
    }

    status_->changeLine(std::move(line));
}

request::UIRequest RoomBrowserScreen::joinSelectedRoom() const {
    if (roomList_ == nullptr) {
        return request::None{};
    }

    const RoomListItem * selected = roomList_->selectedItem();
    if (selected == nullptr) {
        return request::None{};
    }

    return request::RoomRequest{
        request::RoomAction::Join,
        selected->name,
        selectedPassword_ == nullptr ? "" : selectedPassword_->getText()
    };
}

request::UIRequest RoomBrowserScreen::createRoom() const {
    if (createName_ == nullptr) {
        return request::None{};
    }

    const std::string roomName = createName_->getText();
    if (roomName.empty()) {
        return request::None{};
    }

    return request::RoomRequest{
        request::RoomAction::Create,
        roomName,
        createPassword_ == nullptr ? "" : createPassword_->getText()
    };
}

void RoomBrowserScreen::layout() {
    if (widgets.size() <= backButtonIndex_) return;

    const int padding = win_.bordered() ? 2 : 1;
    const int contentX = padding;
    const int contentY = padding;
    const int contentWidth = std::max(1, win_.width() - padding * 2);
    const int contentHeight = std::max(1, win_.height() - padding * 2);
    const int gap = 1;
    const int buttonHeight = 3;
    const int fieldOuterHeight = 3;
    const int statusHeight = 1;
    const int roomsWidth = std::clamp(contentWidth / 2, 32, std::max(32, contentWidth - 38));
    const int sideX = contentX + roomsWidth + gap;
    const int sideWidth = std::max(1, contentX + contentWidth - sideX);
    const int bottomY = contentY + contentHeight - buttonHeight;
    const int statusY = std::max(contentY, bottomY - gap - statusHeight);
    const int bodyHeight = std::max(1, statusY - contentY - gap);
    const int fieldWidth = std::max(1, sideWidth - 2);

    widgets[roomListIndex_]->setRect({
        contentY + 2,
        contentX + 1,
        std::max(1, bodyHeight - 3),
        std::max(1, roomsWidth - 2)
    });

    int fieldY = contentY + 2;
    widgets[selectedPasswordIndex_]->setRect({
        fieldY,
        sideX + 1,
        1,
        fieldWidth
    });
    fieldY += fieldOuterHeight + gap;
    widgets[createNameIndex_]->setRect({
        fieldY,
        sideX + 1,
        1,
        fieldWidth
    });
    fieldY += fieldOuterHeight + gap;
    widgets[createPasswordIndex_]->setRect({
        fieldY,
        sideX + 1,
        1,
        fieldWidth
    });

    widgets[statusIndex_]->setRect({
        statusY,
        contentX,
        statusHeight,
        contentWidth
    });

    const int buttonWidth = std::min(18, std::max(10, (contentWidth - gap * 2) / 3));
    widgets[joinButtonIndex_]->setRect(innerRect({bottomY, contentX, buttonHeight, buttonWidth}));
    widgets[createButtonIndex_]->setRect(innerRect({bottomY, contentX + buttonWidth + gap, buttonHeight, buttonWidth}));
    widgets[backButtonIndex_]->setRect(innerRect({
        bottomY,
        contentX + contentWidth - buttonWidth,
        buttonHeight,
        buttonWidth
    }));
}

void RoomBrowserScreen::resize() {
    layout();
}

request::UIRequest RoomBrowserScreen::handleInput(int key) {
    if (key == 27) {
        return request::RoomRequest{request::RoomAction::Back, "", ""};
    }

    const InputResult result = handleFocusedInput(key);
    if (!std::holds_alternative<request::None>(result.request)) {
        return result.request;
    }

    if (result.handled) {
        updateStatus();
        return request::None{};
    }

    switch (key) {
        case KEY_UP:
        case KEY_BTAB:
            focusPrev();
            updateStatus();
            return request::None{};

        case KEY_DOWN:
        case '\t':
            focusNext();
            updateStatus();
            return request::None{};

        case KEY_LEFT:
            focusWidget(roomListIndex_);
            updateStatus();
            return request::None{};

        case KEY_RIGHT:
            focusWidget(selectedPasswordIndex_);
            updateStatus();
            return request::None{};

        case KEY_ENTER:
        case '\n':
        case '\r':
            if (focusedIndex_ == roomListIndex_) {
                return joinSelectedRoom();
            }
            return request::None{};

        case 'j':
        case 'J':
            return joinSelectedRoom();

        case 'c':
        case 'C':
            focusWidget(createNameIndex_);
            updateStatus();
            return request::None{};

        case 'b':
        case 'B':
            return request::RoomRequest{request::RoomAction::Back, "", ""};
    }

    return request::None{};
}

}  // namespace plague::ui
