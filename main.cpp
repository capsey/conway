#include "main.hpp"
#include "logger.hpp"

#include <cmath>
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
    LogLevel level = LogLevel::ERROR;

    if (info)
        level = LogLevel::INFO;

    if (debug)
        level = LogLevel::DEBUG;

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
                    return lifeBoard.tick();
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
    window.draw(BitBoardRenderer(eraseBuffer, background));
}

inline static sf::Vector2i floor(sf::Vector2f p)
{
    return {(int)std::floor(p.x), (int)std::floor(p.y)};
}

// https://dedu.fr/projects/bresenham/
inline static void gridTraversal(sf::Vector2f p1, sf::Vector2f p2, std::function<void(sf::Vector2i)> func)
{
    int ystep, xstep;
    sf::Vector2i d = floor(p2) - floor(p1);

    if (d.y < 0)
    {
        ystep = -1;
        d.y = -d.y;
    }
    else
        ystep = 1;
    if (d.x < 0)
    {
        xstep = -1;
        d.x = -d.x;
    }
    else
        xstep = 1;

    sf::Vector2i dd = 2 * d;
    sf::Vector2i p = floor(p1);
    func(p);

    if (dd.x >= dd.y)
    {
        int errorprev = d.x;
        int error = d.x;

        for (int i = 0; i < d.x; i++)
        {
            p.x += xstep;
            error += dd.y;

            if (error > dd.x)
            {
                p.y += ystep;
                error -= dd.x;

                if (error + errorprev < dd.x)
                    func({p.x, p.y - ystep});
                else if (error + errorprev > dd.x)
                    func({p.x - xstep, p.y});
                else
                {
                    func({p.x, p.y - ystep});
                    func({p.x - xstep, p.y});
                }
            }

            func(p);
            errorprev = error;
        }
    }
    else
    {
        int errorprev = d.y;
        int error = d.y;

        for (int i = 0; i < d.y; i++)
        {
            p.y += ystep;
            error += dd.x;

            if (error > dd.y)
            {
                p.x += xstep;
                error -= dd.y;

                if (error + errorprev < dd.y)
                    func({p.x - xstep, p.y});
                else if (error + errorprev > dd.y)
                    func({p.x, p.y - ystep});
                else
                {
                    func({p.x - xstep, p.y});
                    func({p.x, p.y - ystep});
                }
            }

            func(p);
            errorprev = error;
        }
    }
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
                return lifeBoard.tick();
            });

        if (event.scancode == sf::Keyboard::Scan::Delete)
            pushTask([](const LifeBoard &lifeBoard)
            {
                return LifeBoard();
            });
    });

    addEventHandler<sf::Event::MouseMoved>([&](const sf::Event::MouseMoved &event)
{
    if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left))
        gridTraversal(worldPos, prevWorldPos, [&](sf::Vector2i pos)
        {
            drawBuffer.set(pos, true);
        });

    if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Right))
        gridTraversal(worldPos, prevWorldPos, [&](sf::Vector2i pos)
        {
            eraseBuffer.set(pos, true);
        });
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

        if (event.button == sf::Mouse::Button::Right)
        {
            pushTask([eraseBuffer = std::move(eraseBuffer)](const LifeBoard &lifeBoard)
            {
                return lifeBoard - eraseBuffer;
            });
            eraseBuffer = BitBoard();
        }
    });
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

        ChunkRenderer::initializeSprites(logger);

        LifeWindow game(logger, 600, 400);
        game.run();
    }
    catch (const Options::Error &error)
    {
        std::osyncstream stream(std::cerr);
        stream << "Error: " << error.what() << "\n";
        stream << "Use '" << error.executable() << " --help' for usage information.\n";
        return 1;
    }
}
