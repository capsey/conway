#pragma once

#include "Logger.hpp"

#include <exception>
#include <string>
#include <string_view>
#include <utility>

constexpr std::string_view CONWAY_VERSION_STRING = "1.0.0";

class Options
{
private:
    std::string m_executable;

public:
    bool help = false;
    bool version = false;
    bool info = false;
    bool debug = false;
    bool benchmark = false;

    // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays, cppcoreguidelines-pro-bounds-pointer-arithmetic, modernize-avoid-c-arrays)
    Options(int argc, char *argv[]);

    void printHelp();
    void printVersion();

    [[nodiscard]] LogLevel getLogLevel() const;

    class Error : public std::exception
    {
    private:
        std::string m_message;
        std::string m_executable;

    public:
        Error(std::string message, std::string executable) : m_message(std::move(message)), m_executable(std::move(executable)) {}

        [[nodiscard]] const char *what() const noexcept override
        {
            return m_message.c_str();
        }

        [[nodiscard]] const std::string &executable() const noexcept
        {
            return m_executable;
        }
    };
};
