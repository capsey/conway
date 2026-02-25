#include "main.hpp"
#include "logger.hpp"
#include "utility.hpp"

#include <chrono>
#include <iostream>
#include <syncstream>

Options::Options(int argc, char *argv[]) : m_executable(argv[0])
{
    bool readingOptions = true;

    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];
        if (arg.empty())
            continue;

        if (readingOptions && arg[0] == '-')
        {
            if (arg == "--")
            {
                readingOptions = false;
                continue;
            }

            if (arg == "-h" || arg == "--help")
            {
                help = true;
                continue;
            }

            if (arg == "-v" || arg == "--version")
            {
                version = true;
                continue;
            }

            if (arg == "--info")
            {
                info = true;
                continue;
            }

            if (arg == "--debug")
            {
                debug = true;
                continue;
            }

            if (arg == "--benchmark")
            {
                benchmark = true;
                continue;
            }

            throw Error("Unknown option '" + arg + "'.", m_executable);
        }
    }
}

void Options::printHelp()
{
    std::osyncstream stream(std::cerr);
    stream << "Usage: " << m_executable << " [OPTIONS]\n";
    stream << "\n";
    stream << "Conway's Game of Life.\n";
    stream << "\n";
    stream << "Options:\n";
    stream << "  -h, --help       Show this help message and exit\n";
    stream << "  -v, --version    Show version and exit\n";
    stream << "  --info           Show more logging information\n";
    stream << "  --debug          Show debugging information\n";
    stream << "  --               Stop parsing options (treat following arguments as filename)\n";
}

void Options::printVersion()
{
    std::osyncstream stream(std::cout);
    stream << "conway " << CONWAY_VERSION_STRING << "\n";
    stream << "sfml " << SFML_VERSION_MAJOR << "." << SFML_VERSION_MINOR << "." << SFML_VERSION_PATCH << "\n";
}

LogLevel Options::getLogLevel()
{
    LogLevel level = LogLevel::Error;

    if (info)
        level = LogLevel::Info;

    if (debug)
        level = LogLevel::Debug;

    return level;
}

std::shared_ptr<BitBoard> Simulation::acquire()
{
    BitBoard *object = nullptr;

    {
        std::lock_guard lock(m_poolMutex);

        if (!m_pool.empty())
        {
            object = m_pool.back();
            m_pool.pop_back();
        }
    }

    if (!object)
        object = new BitBoard();

    std::weak_ptr<Simulation> weak_simulation = shared_from_this();

    return std::shared_ptr<BitBoard>(object, [weak_simulation](BitBoard *object)
    {
        if (auto simulation = weak_simulation.lock())
        {
            std::lock_guard lock(simulation->m_poolMutex);
            simulation->m_pool.push_back(object);
        }
        else
        {
            delete object;
        }
    });
}

void Simulation::clear()
{
    std::lock_guard lock(m_poolMutex);

    for (size_t i = 0; i < m_pool.size(); i++)
        delete m_pool[i];

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
                tick(*m_data.load(), *buffer);
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
        std::lock_guard lock(m_exceptionMutex);
        m_exception = std::current_exception();
    }
}

void Simulation::pushTask(std::function<std::shared_ptr<const BitBoard>()> task)
{
    std::lock_guard lock(m_tickingMutex);
    m_taskQueue.emplace(task);
    m_tickingCondition.notify_all();
}

Simulation::~Simulation()
{
    clear();
}

void Simulation::start()
{
    logger.info("Starting the ticking thread...");
    m_thread = std::thread(&Simulation::tickingThread, this);
}

bool Simulation::togglePause()
{
    logger.debug("Toggling pause state of the simulation.");
    std::lock_guard lock(m_tickingMutex);
    m_paused = !m_paused;
    m_tickingCondition.notify_all();
    return m_paused;
}

void Simulation::scheduleStep()
{
    pushTask([&]()
    {
        std::shared_ptr<BitBoard> buffer = acquire();
        tick(*m_data.load(), *buffer);
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
        std::lock_guard lock(m_tickingMutex);
        m_running = false;
        m_tickingCondition.notify_all();
    }

    logger.info("Joining the ticking thread...");
    m_thread.join();
}

void LifeWindow::initialize()
{
    simulation->start();
}

void LifeWindow::deinitialize()
{
    simulation->stop();
}

void LifeWindow::update()
{
    if (simulation->exception())
        window.close();
}

void LifeWindow::draw()
{
    window.draw(BitBoardRenderer(*simulation->snapshot(), CellColor));
    window.draw(BitBoardRenderer(drawBuffer, CellColor));
}

LifeWindow::LifeWindow(Logger &logger, unsigned int width, unsigned int height) : Window(logger, width, height, "Conway's Game of Life", BackgroundColor), simulation(std::make_shared<Simulation>(logger))
{
    addEventHandler<sf::Event::KeyPressed>([&](const sf::Event::KeyPressed &event)
    {
        if (event.scancode == sf::Keyboard::Scan::Space)
        {
            bool paused = simulation->togglePause();
            background = paused ? PausedColor : BackgroundColor;
        }

        if (event.scancode == sf::Keyboard::Scan::Right)
            simulation->scheduleStep();

        if (event.scancode == sf::Keyboard::Scan::Delete)
            simulation->scheduleClear();
    });

    addEventHandler<sf::Event::MouseButtonPressed>([&](const sf::Event::MouseButtonPressed &event)
    {
        if (event.button == sf::Mouse::Button::Left)
            drawBuffer.set(floor(worldPos), true);

        if (event.button == sf::Mouse::Button::Right)
            simulation->scheduleModify([worldPos = worldPos](BitBoard &lifeBoard)
            {
                lifeBoard.set(floor(worldPos), false);
            });
    });

    addEventHandler<sf::Event::MouseMoved>([&](const sf::Event::MouseMoved &)
    {
        if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left))
            gridTraversal(worldPos, prevWorldPos, [&](sf::Vector2i pos)
            {
                drawBuffer.set(pos, true);
            });

        if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Right))
        {
            BitBoard eraseBuffer;
            gridTraversal(worldPos, prevWorldPos, [&](sf::Vector2i pos)
            {
                eraseBuffer.set(pos, true);
            });

            simulation->scheduleModify([eraseBuffer = std::move(eraseBuffer)](BitBoard &lifeBoard)
            {
                lifeBoard -= eraseBuffer;
            });
        }
    });

    addEventHandler<sf::Event::MouseButtonReleased>([&](const sf::Event::MouseButtonReleased &event)
    {
        if (event.button == sf::Mouse::Button::Left)
        {
            simulation->scheduleModify([drawBuffer = std::move(drawBuffer)](BitBoard &lifeBoard)
            {
                lifeBoard |= drawBuffer;
            });

            drawBuffer = BitBoard();
        }
    });
}

static void runBenchmark(const Options &, Logger &logger)
{
    constexpr int Iterations = 1'000;
    constexpr int StripeLength = 4096;

    BitBoard previousBoard;
    BitBoard currentBoard;

    logger.info("Starting benchmark with {} iterations.", Iterations);

    for (int i = 0; i < StripeLength; i++)
    {
        currentBoard.set({i, 0}, true);
    }

    logger.debug("Initial board seeded with {} live cells.", StripeLength);

    auto t1 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < Iterations; i++)
    {
        std::swap(previousBoard, currentBoard);
        tick(previousBoard, currentBoard);
    }
    auto t2 = std::chrono::high_resolution_clock::now();

    logger.debug("Last generation tick value is {}.", previousBoard.getGeneration());

    std::chrono::duration<double, std::milli> duration = t2 - t1;
    double throughput = 1000.0 * (double)Iterations / duration.count();

    std::osyncstream stream(std::cout);
    stream << "Processed " << Iterations << " iterations in " << duration.count() << " ms\n";
    stream << "Throughput is " << throughput << " iterations per second\n";
}

static void runWindow(const Options &, Logger &logger)
{
    ChunkRenderer::initializeSprites(logger);

    LifeWindow game(logger, 600, 400);
    game.run();
}

int main(int argc, char *argv[])
{
    try
    {
        Options options(argc, argv);

        if (options.help)
        {
            options.printHelp();
            return 0;
        }

        if (options.version)
        {
            options.printVersion();
            return 0;
        }

        Logger logger(options.getLogLevel(), std::cerr);

        if (options.benchmark)
        {
            runBenchmark(options, logger);
            return 0;
        }
        else
        {
            runWindow(options, logger);
            return 0;
        }
    }
    catch (const Options::Error &error)
    {
        std::osyncstream stream(std::cerr);
        stream << "Error: " << error.what() << '\n';
        stream << "Use '" << error.executable() << " --help' for usage information.\n";
        return 1;
    }
}
