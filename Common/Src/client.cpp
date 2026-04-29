#include "UIManager.hpp"
#include <exception>
#include <iostream>

using namespace plague::ui;

int main() {
    try {
        UIManager ui;
        while (1) ui.loop({});
    } catch (const std::exception & err) {
        std::cerr << err.what() << '\n';
        return 1;
    }
}
