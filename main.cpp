#include "main.hpp"

#include <cmath>

void LifeWindow::tickingThread()
{
    std::unique_lock lock(mutex);

    while (running)
    {
        lock.unlock();
        lifeBoard.modify([](const LifeBoard &lifeBoard)
        {
            return lifeBoard.tick();
        });
        lock.lock();

        if (step > 0)
            step--;

        condition.wait(lock, [&]
        {
            return !running || !paused || step > 0;
        });
    }
}

LifeWindow::LifeWindow(unsigned int width, unsigned int height) : Window(width, height, "Conway's Game of Life", BackgroundColor), thread(&LifeWindow::tickingThread, this)
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
        {
            std::lock_guard lock(mutex);
            step++;
            condition.notify_all();
        }

        if (event.scancode == sf::Keyboard::Scan::Delete)
            lifeBoard.reset();
    });

    addEventHandler<sf::Event::MouseButtonReleased>([&](const sf::Event::MouseButtonReleased &event)
    {
        if (event.button == sf::Mouse::Button::Left)
        {
            lifeBoard.modify([&](const LifeBoard &lifeBoard)
            {
                return lifeBoard | *drawBuffer.getData();
            });
            drawBuffer.reset();
        }

        if (event.button == sf::Mouse::Button::Right)
        {
            lifeBoard.modify([&](const LifeBoard &lifeBoard)
            {
                return lifeBoard - *eraseBuffer.getData();
            });
            eraseBuffer.reset();
        }
    });
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

void LifeWindow::update()
{
    if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left))
        gridTraversal(worldPos, prevWorldPos, [&](sf::Vector2i pos)
        {
            drawBuffer.modify([=](const BitBoard &drawBuffer)
            {
                return drawBuffer.set(pos, true);
            });
        });

    if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Right))
        gridTraversal(worldPos, prevWorldPos, [&](sf::Vector2i pos)
        {
            eraseBuffer.modify([=](const BitBoard &eraseBuffer)
            {
                return eraseBuffer.set(pos, true);
            });
        });
}

void LifeWindow::draw()
{
    std::shared_ptr<const LifeBoard> lifeBoardPtr = lifeBoard.getData();
    window.draw(LifeBoardRenderer(*lifeBoardPtr, CellColor));

    std::shared_ptr<const BitBoard> drawBufferPtr = drawBuffer.getData();
    window.draw(BitBoardRenderer(*drawBufferPtr, CellColor));

    std::shared_ptr<const BitBoard> eraseBufferPtr = eraseBuffer.getData();
    window.draw(BitBoardRenderer(*eraseBufferPtr, background));
}

LifeWindow::~LifeWindow()
{
    {
        std::lock_guard lock(mutex);
        running = false;
        condition.notify_all();
    }

    thread.join();
}

int main(int argc, char *argv[])
{
    ChunkRenderer::initializeSprites();

    LifeWindow game(600, 400);
    game.run();
}
