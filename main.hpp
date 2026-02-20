#pragma once

#include "conway.hpp"
#include "window.hpp"

#include <condition_variable>
#include <queue>
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
    bool benchmark = false;

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

class Simulation
{
private:
    std::atomic<std::shared_ptr<const LifeBoard>> m_data;

    bool m_running = true;
    bool m_paused = false;
    std::queue<std::function<LifeBoard(const LifeBoard &)>> m_taskQueue;

    std::thread m_thread;
    std::mutex m_mutex;
    std::condition_variable m_condition;

    void tickingThread();

protected:
    Logger &logger;

public:
    Simulation(Logger &logger) : m_data(std::make_shared<const LifeBoard>()), logger(logger) {}
    Simulation(Logger &logger, LifeBoard data) : m_data(std::make_shared<const LifeBoard>(std::move(data))), logger(logger) {}

    void start();
    bool togglePause();
    void pushTask(std::function<LifeBoard(const LifeBoard &)> task);
    void stop();

    std::shared_ptr<const LifeBoard> get()
    {
        return m_data.load();
    }

    void reset()
    {
        m_data.store(std::make_shared<const LifeBoard>());
    }
};

class LifeWindow : public Window
{
protected:
    Simulation simulation;
    BitBoard drawBuffer;

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
