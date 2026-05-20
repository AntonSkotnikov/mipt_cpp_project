#pragma once

#include "ScreenManager.hpp"
#include "Settings.hpp"
#include "UIRequest.hpp"
#include "UI_ClientAPI.hpp"
#include <memory>

namespace plague::ui {

/**
 * @brief Top-level ncurses UI controller.
 *
 * UIManager owns the screen manager, initializes and shuts down ncurses, routes
 * snapshots to screens, and converts user input into UI requests for the client.
 */
class UIManager final {
private:
    std::unique_ptr<ScreenManager> man_;
    Config        cfg_;
    GameSnapshot snap_;

public:
    /** @brief Initialize ncurses, color pairs, terminal profile, and screens. */
    UIManager(); // init of ncurses etc
    /** @brief Restore terminal state and release all UI resources. */
    ~UIManager();
    /**
     * @brief Run one UI frame.
     * @param snap Latest game snapshot from the client state.
     * @return Request produced by the active screen, or request::None.
     */
    request::UIRequest loop(GameSnapshot snap);
    /** @brief Recompute terminal profile, resize windows, and redraw. */
    void resize();
};

}
