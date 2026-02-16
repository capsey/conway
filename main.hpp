#pragma once

#include "conway.hpp"
#include "window.hpp"

#include <condition_variable>
#include <thread>

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

    Options(int argc, char *argv[]);

    void printHelp();
    void printVersion();

    LogLevel getLogLevel();

    class Error : public std::exception
    {
    private:
        std::string m_message;
        std::string m_executable;

    public:
        Error(std::string message, std::string executable) : m_message(std::move(message)), m_executable(std::move(executable)) {}

        const char *what() const noexcept override
        {
            return m_message.c_str();
        }

        const std::string &executable() const noexcept
        {
            return m_executable;
        }
    };
};

class LifeWindow : public Window
{
protected:
    Container<LifeBoard> lifeBoard;
    Container<BitBoard> drawBuffer;
    Container<BitBoard> eraseBuffer;

    std::thread thread;
    std::mutex mutex;
    std::condition_variable condition;

    bool running = true;
    bool paused = false;
    int step = 0;

    void tickingThread();

    void initialize() override;
    void deinitialize() override;

    void update() override;
    void draw() override;

public:
    static constexpr sf::Color BackgroundColor = sf::Color::Black;
    static constexpr sf::Color PausedColor = sf::Color(32, 32, 32);
    static constexpr sf::Color CellColor = sf::Color::White;

    LifeWindow(Logger &logger, unsigned int width, unsigned int height);
};
