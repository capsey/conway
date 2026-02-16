#pragma once

#include <format>
#include <iostream>

enum class LogLevel
{
    DEBUG = 0,
    INFO,
    ERROR,
};

class Logger
{
private:
    LogLevel m_level;
    std::ostream &m_out;

    void log(LogLevel level, std::string_view message);

    template <typename... Args>
    void log(LogLevel level, std::format_string<Args...> fmt, Args &&...args);

public:
    Logger(LogLevel level, std::ostream &out) : m_level(level), m_out(out) {}

    inline void debug(std::string_view message) { log(LogLevel::DEBUG, message); }

    template <typename... Args>
    inline void debug(std::format_string<Args...> fmt, Args &&...args) { log(LogLevel::DEBUG, fmt, std::forward<Args>(args)...); }

    inline void info(std::string_view message) { log(LogLevel::INFO, message); }

    template <typename... Args>
    inline void info(std::format_string<Args...> fmt, Args &&...args) { log(LogLevel::INFO, fmt, std::forward<Args>(args)...); }

    inline void error(std::string_view message) { log(LogLevel::ERROR, message); }

    template <typename... Args>
    inline void error(std::format_string<Args...> fmt, Args &&...args) { log(LogLevel::ERROR, fmt, std::forward<Args>(args)...); }
};
