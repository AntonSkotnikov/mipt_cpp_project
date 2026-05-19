#include "GameRenderer.hpp"

namespace plague {

request::UIRequest GameRenderer::pollInput(const GameState& state) {
    return ui_.loop(state.snapshot()); // Делаем  копию состояния, snapshot во время рендера,
                                       //передаем ее в UI и возвращаем действие пользователя
}

void GameRenderer::render(const GameState& state) {
    (void)state;
}

}
