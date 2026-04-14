#include "ClientApp.hpp"
#include "ConsoleUi.hpp"
#include "DummyTransport.hpp"

int main() {
    plague::ConsoleUi ui;
    plague::DummyTransport transport;
    plague::ClientApp app(ui, transport);
    app.run();
    return 0;
}
