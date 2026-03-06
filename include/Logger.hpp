#pragma once

#include <chrono>
#include <cstdint>
#include <format>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string_view>
#include <syncstream>
#include <thread>

enum class LogLevel : uint8_t
{
    Debug = 0,
    Info,
    Warning,
    Error,
};

constexpr static std::string_view levelString(LogLevel level)
{
    switch (level)
    {
    case LogLevel::Debug:
        return "DEBUG";

    case LogLevel::Info:
        return "INFO";

    case LogLevel::Warning:
        return "WARNING";

    case LogLevel::Error:
        return "ERROR";

    default:
        throw std::invalid_argument("unknown logging level");
    }
}

inline static void printHeader(LogLevel level, std::osyncstream &stream)
{
    auto now = std::chrono::system_clock::now();

    stream << '[';
    std::format_to(std::ostream_iterator<char>(stream), "{:%F %T %Z}", now);
    stream << "] [" << levelString(level) << "] ";
    stream << '[' << std::this_thread::get_id() << "] ";
}

class Logger
{
private:
    LogLevel m_level;
    std::ostream &m_out;

    void log(LogLevel level, std::string_view message)
    {
        if (level < m_level)
            return;

        std::osyncstream stream(m_out);

        printHeader(level, stream);
        stream << message << '\n';
    }

    template <typename... Args>
    void log(LogLevel level, std::format_string<Args...> fmt, Args &&...args)
    {
        if (level < m_level)
            return;

        std::osyncstream stream(m_out);

        printHeader(level, stream);
        std::format_to(std::ostream_iterator<char>(stream), fmt, std::forward<Args>(args)...);
        stream << '\n';
    }

public:
    Logger(LogLevel level, std::ostream &out) : m_level(level), m_out(out) {}

    void debug(std::string_view message) { log(LogLevel::Debug, message); }

    template <typename... Args>
    void debug(std::format_string<Args...> fmt, Args &&...args) { log(LogLevel::Debug, fmt, std::forward<Args>(args)...); }

    void info(std::string_view message) { log(LogLevel::Info, message); }

    template <typename... Args>
    void info(std::format_string<Args...> fmt, Args &&...args) { log(LogLevel::Info, fmt, std::forward<Args>(args)...); }

    void warn(std::string_view message) { log(LogLevel::Warning, message); }

    template <typename... Args>
    void warn(std::format_string<Args...> fmt, Args &&...args) { log(LogLevel::Warning, fmt, std::forward<Args>(args)...); }

    void error(std::string_view message) { log(LogLevel::Error, message); }

    template <typename... Args>
    void error(std::format_string<Args...> fmt, Args &&...args) { log(LogLevel::Error, fmt, std::forward<Args>(args)...); }
};
