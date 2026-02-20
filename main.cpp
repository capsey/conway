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

void LifeWindow::tickingThread()
{
    try
    {
        std::unique_lock lock(mutex);
        logger.info("The ticking thread started.");

        while (running)
        {
            if (!paused)
            {
                lock.unlock();
                lifeBoard.modify([](const LifeBoard &lifeBoard)
                {
                    return lifeBoard.next();
                });
                lock.lock();
            }

            while (!taskQueue.empty())
            {
                auto task = taskQueue.front();
                taskQueue.pop();

                lock.unlock();
                lifeBoard.modify(task);
                lock.lock();
            }

            condition.wait(lock, [&]
            {
                return !running || !paused || !taskQueue.empty();
            });
        }
    }
    catch (const std::exception &e)
    {
        logger.error(e.what());
    }
}

void LifeWindow::pushTask(std::function<LifeBoard(const LifeBoard &)> task)
{
    std::lock_guard lock(mutex);
    taskQueue.emplace(task);
    condition.notify_all();
}

void LifeWindow::initialize()
{
    logger.info("Starting the ticking thread...");
    thread = std::thread(&LifeWindow::tickingThread, this);
}

void LifeWindow::deinitialize()
{
    {
        std::lock_guard lock(mutex);
        running = false;
        condition.notify_all();
    }

    logger.info("Joining the ticking thread...");
    thread.join();
}

void LifeWindow::update()
{
}

void LifeWindow::draw()
{
    window.draw(LifeBoardRenderer(*lifeBoard.get(), CellColor));
    window.draw(BitBoardRenderer(drawBuffer, CellColor));
}

LifeWindow::LifeWindow(Logger &logger, unsigned int width, unsigned int height) : Window(logger, width, height, "Conway's Game of Life", BackgroundColor)
{
    addEventHandler<sf::Event::KeyPressed>([&](const sf::Event::KeyPressed &event)
    {
        if (event.scancode == sf::Keyboard::Scan::Space)
        {
            std::lock_guard lock(mutex);
            paused = !paused;
            background = paused ? PausedColor : BackgroundColor;
            condition.notify_all();
        }

        if (event.scancode == sf::Keyboard::Scan::Right)
            pushTask([](const LifeBoard &lifeBoard)
            {
                return lifeBoard.next();
            });

        if (event.scancode == sf::Keyboard::Scan::Delete)
            pushTask([](const LifeBoard &)
            {
                return LifeBoard();
            });
    });

    addEventHandler<sf::Event::MouseButtonPressed>([&](const sf::Event::MouseButtonPressed &event)
    {
        if (event.button == sf::Mouse::Button::Left)
            drawBuffer.set(floor(worldPos), true);

        if (event.button == sf::Mouse::Button::Right)
            pushTask([worldPos = worldPos](const LifeBoard &lifeBoard)
            {
                LifeBoard result = lifeBoard;
                return result.set(floor(worldPos), false);
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
            pushTask([eraseBuffer = std::move(eraseBuffer)](const LifeBoard &lifeBoard)
            {
                return lifeBoard - eraseBuffer;
            });
        }
    });

    addEventHandler<sf::Event::MouseButtonReleased>([&](const sf::Event::MouseButtonReleased &event)
    {
        if (event.button == sf::Mouse::Button::Left)
        {
            pushTask([drawBuffer = std::move(drawBuffer)](const LifeBoard &lifeBoard)
            {
                return lifeBoard | drawBuffer;
            });
            drawBuffer = BitBoard();
        }
    });
}

static void runBenchmark(const Options &, Logger &logger)
{
    constexpr int Iterations = 1'000;
    constexpr int StripeLength = 4096;

    LifeBoard lifeBoard;

    logger.info("Starting benchmark with {} iterations", Iterations);

    for (int i = 0; i < StripeLength; i++)
    {
        lifeBoard.set({i, 0}, true);
    }

    logger.debug("Initial board seeded with {} live cells", StripeLength);

    auto t1 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < Iterations; i++)
    {
        lifeBoard = lifeBoard.next();
    }
    auto t2 = std::chrono::high_resolution_clock::now();

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
