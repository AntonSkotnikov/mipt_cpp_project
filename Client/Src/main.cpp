#include "ClientApp.hpp"
#include "SocketTransport.hpp"

int main() {
    plague::SocketTransport transport;
    plague::ClientApp app(transport);
    app.run();
    return 0;
}
