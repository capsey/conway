#pragma once

#include "logger.hpp"

#include <exception>
#include <string>
#include <utility>

#define CONWAY_VERSION_STRING "1.0.0"

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
