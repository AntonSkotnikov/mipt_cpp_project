#include "GameRenderer.hpp"

namespace plague {

request::UIRequest GameRenderer::pollInput(const GameState& state) {
    return ui_.loop(state.snapshot());
}

void GameRenderer::render(const GameState& state) {
    (void)state;
}

}
