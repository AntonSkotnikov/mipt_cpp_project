#include "Window.hpp"
#include "Widget.hpp"
#include "Decorator.hpp"

using namespace plague::ui;

int main() {
    initscr();
    noecho();
    cbreak();
    keypad(stdscr, TRUE);
    curs_set(0);

    Window win(20, 60, 2, 5);

    auto root = std::make_unique<Tabs>();

auto mainPanel = std::make_unique<ListBox>();
mainPanel->add(std::make_unique<Button>("Start", [] {}));
mainPanel->add(std::make_unique<Button>("Options", [] {}));

auto settings = std::make_unique<CheckBox>("Enable sound");

root->addTab("Main", std::move(mainPanel));
root->addTab("Settings", std::move(settings));

root->setRect({0, 0, 20, 60});
root->setFocus(true);

while (true) {
    win.clear();

    root->draw(win);

    win.refresh();

    int key = win.getKey();
    root->handleInput(key);
}
}