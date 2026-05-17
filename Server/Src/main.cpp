#include "PlagueServer.hpp"
#include "logger.hpp"

int main() {
    Logger::setConsoleOutputEnabled(false);
    Logger::setDetailedOutputEnabled(false);
    Logger::setMinimumLevel(LogLevel::Info);

    if (!Logger::init("logs/server.log")) {
        LOG_WARNING("Failed to open server log file");
    }

    LOG_INFO("Starting server");

    plague::PlagueServer server("0.0.0.0", 5555);
    server.run();

    Logger::shutdown();
    return 0;
}
