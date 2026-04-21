#pragma once

#include "UIRequest.hpp"

namespace plague::ui {

class Screen {
public:
    virtual ~Screen() = default;

    virtual void draw() const = 0;
    virtual plague::request::UIRequest handleInput(int key) = 0;
};

void drawCenteredText(int y, const char* text);

}