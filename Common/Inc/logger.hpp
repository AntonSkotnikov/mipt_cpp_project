#pragma once

#include <cstdio>
#include <string>
#include <vector>

namespace plague {

enum class LogLevel {
    Debug,
    Info,
    Warning,
    Error,
    Fatal
};

class Logger {
public:
    static bool init(const std::string& filename);
    static void shutdown();
    static void setConsoleOutputEnabled(bool enabled);
    static void setDetailedOutputEnabled(bool enabled);
    static void setMinimumLevel(LogLevel level);
    static void log(LogLevel level,
                    const char* file,
                    int line,
                    const std::string& message);

    template <typename... Args>
    static void logf(LogLevel level,
                     const char* file,
                     int line,
                     const char* format,
                     Args... args) {
        log(level, file, line, formatMessage(format, args...));
    }

private:
    static std::string formatMessage(const char* message) {
        return message == nullptr ? std::string{} : std::string(message);
    }

    template <typename... Args>
    static std::string formatMessage(const char* format, Args... args) {
        if (format == nullptr) {
            return {};
        }

        const int size = std::snprintf(nullptr, 0, format, args...);
        if (size <= 0) {
            return std::string(format);
        }

        std::vector<char> buffer(static_cast<std::size_t>(size) + 1);
        std::snprintf(buffer.data(), buffer.size(), format, args...);
        return std::string(buffer.data(), static_cast<std::size_t>(size));
    }
};

}  // namespace plague

using Logger = plague::Logger;
using LogLevel = plague::LogLevel;

#define LOG_DEBUG(...) ::plague::Logger::logf(::plague::LogLevel::Debug, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_INFO(...) ::plague::Logger::logf(::plague::LogLevel::Info, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_WARNING(...) ::plague::Logger::logf(::plague::LogLevel::Warning, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_ERROR(...) ::plague::Logger::logf(::plague::LogLevel::Error, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_FATAL(...) ::plague::Logger::logf(::plague::LogLevel::Fatal, __FILE__, __LINE__, __VA_ARGS__)
