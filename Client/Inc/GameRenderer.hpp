#pragma once

#include "GameState.hpp"
#include "UIManager.hpp"
#include "UIRequest.hpp"

namespace plague {

// Тонкая обёртка над UIManager, чтобы ClientApp не зависел от деталей UI.
class GameRenderer {
public:
    GameRenderer() = default;

    request::UIRequest pollInput(const GameState& state);
    void render(const GameState& state);

private:
    ui::UIManager ui_;
};

}
