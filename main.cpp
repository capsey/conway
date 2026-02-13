#include "main.hpp"

#include <cmath>

void LifeWindow::tickingThread()
{
    std::unique_lock lock(mutex);

    while (running)
    {
        lock.unlock();
        board.tick();
        lock.lock();

        if (step > 0)
            step--;

        condition.wait(lock, [&]
        {
            return !running || !paused || step > 0;
        });
    }
}

LifeWindow::LifeWindow(unsigned int width, unsigned int height) : Window(width, height, "Conway's Game of Life", BackgroundColor), board(CellColor), drawBuffer(CellColor), eraseBuffer(BackgroundColor), thread(&LifeWindow::tickingThread, this)
{
    addEventHandler<sf::Event::KeyPressed>([&](const sf::Event::KeyPressed &event)
    {
        if (event.scancode == sf::Keyboard::Scan::Space)
        {
            std::lock_guard lock(mutex);
            paused = !paused;
            background = paused ? PausedColor : BackgroundColor;
            eraseBuffer.color = paused ? PausedColor : BackgroundColor;
            condition.notify_all();
        }

        if (event.scancode == sf::Keyboard::Scan::Right)
        {
            std::lock_guard lock(mutex);
            step++;
            condition.notify_all();
        }

        if (event.scancode == sf::Keyboard::Scan::Delete)
            board.clear();
    });

    addEventHandler<sf::Event::MouseButtonReleased>([&](const sf::Event::MouseButtonReleased &event)
    {
        if (event.button == sf::Mouse::Button::Left)
        {
            board |= drawBuffer;
            drawBuffer.clear();
        }

        if (event.button == sf::Mouse::Button::Right)
        {
            board -= eraseBuffer;
            eraseBuffer.clear();
        }
    });
}

inline static void gridTraversal(sf::Vector2f p0, sf::Vector2f p1, std::function<void(sf::Vector2i)> func)
{
    float dx = p1.x - p0.x;
    float dy = p1.y - p0.y;

    int x = static_cast<int>(std::floor(p0.x));
    int y = static_cast<int>(std::floor(p0.y));

    int endX = static_cast<int>(std::floor(p1.x));
    int endY = static_cast<int>(std::floor(p1.y));

    // clang-format off
    int stepX = dx > 0 ? 1 : dx < 0 ? -1 : 0;
    int stepY = dy > 0 ? 1 : dy < 0 ? -1 : 0;

    float tDeltaX = dx != 0.0f ? std::abs(1.0f / dx) : std::numeric_limits<float>::infinity();
    float tDeltaY = dy != 0.0f ? std::abs(1.0f / dy) : std::numeric_limits<float>::infinity();

    float tMaxX = dx > 0 ? (std::floor(p0.x) + 1 - p0.x) / dx : dx < 0 ? (p0.x - std::floor(p0.x)) / -dx : std::numeric_limits<float>::infinity();
    float tMaxY = dy > 0 ? (std::floor(p0.y) + 1 - p0.y) / dy : dy < 0 ? (p0.y - std::floor(p0.y)) / -dy : std::numeric_limits<float>::infinity();
    // clang-format on

    func({x, y});

    while (x != endX || y != endY)
    {
        if (tMaxX < tMaxY)
        {
            x += stepX;
            tMaxX += tDeltaX;
        }
        else
        {
            y += stepY;
            tMaxY += tDeltaY;
        }

        func({x, y});
    }
}

void LifeWindow::update()
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
}

void LifeWindow::draw()
{
    window.draw(board);
    window.draw(drawBuffer);
    window.draw(eraseBuffer);
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
