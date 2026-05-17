#include "ClientApp.hpp"
#include "SocketTransport.hpp"
#include "logger.hpp"

int main() {
    Logger::setConsoleOutputEnabled(false);
    Logger::setDetailedOutputEnabled(false);
    Logger::setMinimumLevel(LogLevel::Info);

    if (!Logger::init("logs/client.log")) {
        LOG_WARNING("Failed to open client log file");
    }

    LOG_INFO("Starting client");

    plague::SocketTransport transport;
    plague::ClientApp app(transport);
    app.run();

    LOG_INFO("Ending client");
    Logger::shutdown();
    return 0;
}
