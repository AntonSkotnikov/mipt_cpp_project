#include "ClientApp.hpp"
#include "InterfaceUiAdapter.hpp"
#include "SocketTransport.hpp"

int main() {
    plague::InterfaceUiAdapter ui;
    plague::SocketTransport transport;
    plague::ClientApp app(ui, transport);
    app.run();
    return 0;
}
