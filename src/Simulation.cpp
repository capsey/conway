#include "Simulation.hpp"
#include "BitBoard.hpp"
#include "conway.hpp"

#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <utility>

std::shared_ptr<BitBoard> Simulation::acquire()
{
    std::unique_ptr<BitBoard> object;

    {
        std::scoped_lock lock(m_poolMutex);

        if (!m_pool.empty())
        {
            object = std::move(m_pool.back());
            m_pool.pop_back();
        }
    }

    if (!object)
    {
        logger.info("Object pool is empty, creating new BitBoard.");
        object = std::make_unique<BitBoard>();
    }

    std::weak_ptr<Simulation> weakSimulation = shared_from_this();

    return {object.release(), [weakSimulation](BitBoard *ptr)
    {
        std::unique_ptr<BitBoard> object(ptr);

        if (auto simulation = weakSimulation.lock())
        {
            std::scoped_lock lock(simulation->m_poolMutex);
            simulation->m_pool.push_back(std::move(object));
        }
    }};
}

void Simulation::clear()
{
    logger.info("Clearing the object pool.");
    std::scoped_lock lock(m_poolMutex);
    m_pool.clear();
}

void Simulation::tickingThread()
{
    try
    {
        std::unique_lock lock(m_tickingMutex);
        logger.info("The ticking thread started.");

        while (m_running)
        {
            if (!m_paused)
            {
                lock.unlock();
                std::shared_ptr<BitBoard> buffer = acquire();
                conway::tick(*m_data.load(), *buffer);
                m_data.store(buffer);
                lock.lock();
            }

            while (!m_taskQueue.empty())
            {
                auto task = m_taskQueue.front();
                m_taskQueue.pop();

                lock.unlock();
                m_data.store(task());
                lock.lock();
            }

            m_tickingCondition.wait(lock, [&]
            {
                return !m_running || !m_paused || !m_taskQueue.empty();
            });
        }
    }
    catch (const std::exception &e)
    {
        logger.error(e.what());

        std::scoped_lock lock(m_exceptionMutex);
        m_exception = std::current_exception();
    }
}

void Simulation::pushTask(const std::function<std::shared_ptr<const BitBoard>()> &task)
{
    logger.debug("Pushing a new task to the task queue.");

    std::scoped_lock lock(m_tickingMutex);
    m_taskQueue.emplace(task);
    m_tickingCondition.notify_all();
}

void Simulation::start()
{
    logger.info("Starting the ticking thread...");
    m_thread = std::thread(&Simulation::tickingThread, this);
}

bool Simulation::togglePause()
{
    logger.debug("Toggling pause state of the simulation.");
    std::scoped_lock lock(m_tickingMutex);
    m_paused = !m_paused;
    m_tickingCondition.notify_all();
    return m_paused;
}

void Simulation::scheduleStep()
{
    pushTask([&]()
    {
        std::shared_ptr<BitBoard> buffer = acquire();
        conway::tick(*m_data.load(), *buffer);
        return buffer;
    });
}

void Simulation::scheduleModify(std::function<void(BitBoard &)> func)
{
    pushTask([this, func = std::move(func)]()
    {
        std::shared_ptr<BitBoard> buffer = acquire();
        *buffer = *m_data.load();
        func(*buffer);
        return buffer;
    });
}

void Simulation::scheduleClear()
{
    pushTask([&]()
    {
        clear();
        return acquire();
    });
}

void Simulation::stop()
{
    {
        std::scoped_lock lock(m_tickingMutex);
        m_running = false;
        m_tickingCondition.notify_all();
    }

    logger.info("Joining the ticking thread...");
    m_thread.join();
}
