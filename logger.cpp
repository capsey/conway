#include "logger.hpp"

#include <iomanip>
#include <iostream>
#include <iterator>
#include <syncstream>
#include <thread>

inline static std::string_view levelString(LogLevel level)
{
    switch (level)
    {
    case LogLevel::DEBUG:
        return "DEBUG";

    case LogLevel::INFO:
        return "INFO";

    case LogLevel::ERROR:
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

void Logger::log(LogLevel level, std::string_view message)
{
    if (level < m_level)
        return;

    std::osyncstream stream(m_out);

    printHeader(level, stream);
    stream << message << '\n';
}

template <typename... Args>
void Logger::log(LogLevel level, std::format_string<Args...> fmt, Args &&...args)
{
    if (level < m_level)
        return;

    std::osyncstream stream(m_out);

    printHeader(level, stream);
    std::format_to(std::ostream_iterator<char>(stream), fmt, std::forward<Args>(args)...);
    stream << '\n';
}
