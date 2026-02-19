#pragma once

#include <format>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <syncstream>
#include <thread>

enum class LogLevel
{
    Debug = 0,
    Info,
    Error,
};

inline static std::string_view levelString(LogLevel level)
{
    switch (level)
    {
    case LogLevel::Debug:
        return "DEBUG";

    case LogLevel::Info:
        return "INFO";

    case LogLevel::Error:
        return "ERROR";

    default:
        return "???";
    }
}

inline static void printHeader(LogLevel level, std::osyncstream &stream)
{
    std::time_t time = std::time(nullptr);
    std::tm tm = *std::localtime(&time);

    stream << "[" << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << "] ";
    stream << "[" << levelString(level) << "] ";
    stream << "[" << std::this_thread::get_id() << "] ";
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

    inline void debug(std::string_view message) { log(LogLevel::Debug, message); }

    template <typename... Args>
    inline void debug(std::format_string<Args...> fmt, Args &&...args) { log(LogLevel::Debug, fmt, std::forward<Args>(args)...); }

    inline void info(std::string_view message) { log(LogLevel::Info, message); }

    template <typename... Args>
    inline void info(std::format_string<Args...> fmt, Args &&...args) { log(LogLevel::Info, fmt, std::forward<Args>(args)...); }

    inline void error(std::string_view message) { log(LogLevel::Error, message); }

    template <typename... Args>
    inline void error(std::format_string<Args...> fmt, Args &&...args) { log(LogLevel::Error, fmt, std::forward<Args>(args)...); }
};
