#pragma once

#include "BitBoard.hpp"
#include "Logger.hpp"

#include <atomic>
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

    std::vector<std::unique_ptr<BitBoard>> m_pool;
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

    Simulation(const Simulation &) = delete;
    Simulation &operator=(const Simulation &) = delete;
    Simulation(Simulation &&) = delete;
    Simulation &operator=(Simulation &&) = delete;

    ~Simulation() = default;

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
