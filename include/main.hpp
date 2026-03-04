#pragma once

#include "bitboard.hpp"
#include "logger.hpp"
#include "window.hpp"

#include <SFML/Graphics/Color.hpp>
#include <condition_variable>
#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <utility>
#include <vector>

class Simulation : public std::enable_shared_from_this<Simulation>
{
private:
    std::thread m_thread;

    std::atomic<std::shared_ptr<const BitBoard>> m_data;

    bool m_running = true;
    bool m_paused = false;
    std::queue<std::function<std::shared_ptr<const BitBoard>()>> m_taskQueue;
    std::mutex m_tickingMutex;
    std::condition_variable m_tickingCondition;

    std::vector<BitBoard *> m_pool;
    std::mutex m_poolMutex;

    std::exception_ptr m_exception;
    std::mutex m_exceptionMutex;

    [[nodiscard]] std::shared_ptr<BitBoard> acquire();
    void clear();

    void tickingThread();
    void pushTask(const std::function<std::shared_ptr<const BitBoard>()> &task);

protected:
    Logger &logger;

public:
    Simulation(Logger &logger) : m_data(std::make_shared<const BitBoard>()), logger(logger) {}
    Simulation(Logger &logger, BitBoard data) : m_data(std::make_shared<const BitBoard>(std::move(data))), logger(logger) {}
    ~Simulation();

    void start();
    bool togglePause();
    void scheduleStep();
    void scheduleModify(std::function<void(BitBoard &)> func);
    void scheduleClear();
    void stop();

    [[nodiscard]] std::shared_ptr<const BitBoard> snapshot()
    {
        return m_data.load();
    }

    [[nodiscard]] std::exception_ptr exception()
    {
        std::scoped_lock lock(m_exceptionMutex);
        return m_exception;
    }
};

class LifeWindow : public Window
{
protected:
    std::shared_ptr<Simulation> simulation;
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
