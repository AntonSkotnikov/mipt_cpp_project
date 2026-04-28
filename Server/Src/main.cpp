#include "PlagueServer.hpp"

int main() {
    plague::PlagueServer server("0.0.0.0", 5555);
    server.run();
    return 0;
}