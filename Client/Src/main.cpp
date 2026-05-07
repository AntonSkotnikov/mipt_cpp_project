#include "ClientApp.hpp"
#include "UIManager.hpp"
#include "SocketTransport.hpp"

int main() {
    plague::ui::UIManager ui;
    plague::SocketTransport transport;
    plague::ClientApp app(ui, transport);
    app.run();
    return 0;
}
