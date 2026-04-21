#include "ClientApp.hpp"
#include "DummyTransport.hpp"
#include "InterfaceUiAdapter.hpp"

int main() {
    plague::InterfaceUiAdapter ui;
    plague::DummyTransport transport;
    plague::ClientApp app(ui, transport);
    app.run();
    return 0;
}
