#include "logger.hpp"

#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <thread>

namespace plague {

namespace {

std::mutex& loggerMutex() {
    static std::mutex mutex;
    return mutex;
}

std::fstream& logFile() {
    static std::fstream file;
    return file;
}

bool& consoleOutputEnabled() {
    static bool enabled = true;
    return enabled;
}

bool& detailedOutputEnabled() {
    static bool enabled = true;
    return enabled;
}

LogLevel& minimumLevel() {
    static LogLevel level = LogLevel::Debug;
    return level;
}

int levelPriority(LogLevel level) {
    switch (level) {
    case LogLevel::Debug:
        return 0;
    case LogLevel::Info:
        return 1;
    case LogLevel::Warning:
        return 2;
    case LogLevel::Error:
        return 3;
    case LogLevel::Fatal:
        return 4;
    }

    return 1;
}

const char* levelName(LogLevel level) {
    switch (level) {
    case LogLevel::Debug:
        return "DEBUG";
    case LogLevel::Info:
        return "INFO";
    case LogLevel::Warning:
        return "WARNING";
    case LogLevel::Error:
        return "ERROR";
    case LogLevel::Fatal:
        return "FATAL";
    }

    return "INFO";
}

bool usesStdErr(LogLevel level) {
    return level == LogLevel::Warning ||
           level == LogLevel::Error ||
           level == LogLevel::Fatal;
}

std::tm localTime(std::time_t time) {
    std::tm result {};
#if defined(_WIN32)
    localtime_s(&result, &time);
#else
    localtime_r(&time, &result);
#endif
    return result;
}

std::string timestamp() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t time = std::chrono::system_clock::to_time_t(now);
    const std::tm local = localTime(time);

    std::ostringstream stream;
    stream << std::put_time(&local, "%Y-%m-%d %H:%M:%S");
    return stream.str();
}

const char* shortFileName(const char* file) {
    if (file == nullptr) {
        return "";
    }

    const char* lastSlash = file;
    for (const char* current = file; *current != '\0'; ++current) {
        if (*current == '/' || *current == '\\') {
            lastSlash = current + 1;
        }
    }

    return lastSlash;
}

void ensureLogDirectoryExists(const std::string& filename) {
    const std::filesystem::path path(filename);
    const std::filesystem::path parent = path.parent_path();
    if (parent.empty()) {
        return;
    }

    std::error_code error;
    std::filesystem::create_directories(parent, error);
}

}  // namespace

bool Logger::init(const std::string& filename) {
    std::lock_guard<std::mutex> lock(loggerMutex());

    ensureLogDirectoryExists(filename);

    std::fstream& file = logFile();
    if (file.is_open()) {
        file.close();
    }

    file.open(filename, std::ios::out | std::ios::app);
    return file.is_open();
}

void Logger::shutdown() {
    std::lock_guard<std::mutex> lock(loggerMutex());

    std::fstream& file = logFile();
    if (file.is_open()) {
        file.close();
    }
}

void Logger::setConsoleOutputEnabled(bool enabled) {
    std::lock_guard<std::mutex> lock(loggerMutex());
    consoleOutputEnabled() = enabled;
}

void Logger::setDetailedOutputEnabled(bool enabled) {
    std::lock_guard<std::mutex> lock(loggerMutex());
    detailedOutputEnabled() = enabled;
}

void Logger::setMinimumLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(loggerMutex());
    minimumLevel() = level;
}

void Logger::log(LogLevel level,
                 const char* file,
                 int line,
                 const std::string& message) {
    std::lock_guard<std::mutex> lock(loggerMutex());

    if (levelPriority(level) < levelPriority(minimumLevel())) {
        return;
    }

    std::ostringstream entry;
    entry << '[' << timestamp() << "] "
          << '[' << levelName(level) << "] ";

    if (detailedOutputEnabled()) {
        entry << '[' << std::this_thread::get_id() << "] "
              << '[' << shortFileName(file) << ':' << line << "] ";
    }

    entry << message;

    if (consoleOutputEnabled()) {
        std::ostream& console = usesStdErr(level) ? std::cerr : std::cout;
        console << entry.str() << '\n';
        console.flush();
    }

    std::fstream& output = logFile();
    if (output.is_open()) {
        output << entry.str() << '\n';
        output.flush();
    }
}

}  // namespace plague
