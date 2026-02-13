#pragma once

#include "conway.hpp"
#include "window.hpp"

#include <condition_variable>
#include <thread>

class LifeWindow : public Window
{
protected:
    static constexpr sf::Color BackgroundColor = sf::Color::Black;
    static constexpr sf::Color PausedColor = sf::Color(32, 32, 32);
    static constexpr sf::Color CellColor = sf::Color::White;

    LifeBoard board;
    BitBoard drawBuffer;
    BitBoard eraseBuffer;

    std::mutex mutex;
    std::condition_variable condition;
    std::thread thread;

    bool running = true;
    bool paused = false;
    int step = 0;

    void tickingThread();

    void update() override;
    void draw() override;

public:
    LifeWindow(unsigned int width, unsigned int height);
    ~LifeWindow();
};
