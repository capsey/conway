#pragma once

#include "Logger.hpp"

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/View.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Window/Event.hpp>
#include <functional>
#include <string>
#include <utility>
#include <vector>

class Window
{
private:
    std::vector<std::function<void(const sf::Event &)>> m_handlers;

protected:
    Logger &logger;

    sf::RenderWindow window;
    sf::Vector2u size;
    sf::View view;
    sf::Color background = sf::Color::Black;

    sf::Vector2i mousePos;
    sf::Vector2i prevMousePos;
    sf::Vector2f worldPos;
    sf::Vector2f prevWorldPos;

    template <typename T>
    void addEventHandler(std::function<void(const T &)> handler)
    {
        m_handlers.emplace_back([handler = std::move(handler)](const sf::Event &event)
        {
            if (const auto *t = event.getIf<T>())
                handler(*t);
        });
    }

    virtual void initialize() = 0;
    virtual void deinitialize() = 0;

    virtual void update() = 0;
    virtual void draw() = 0;

public:
    Window(Logger &logger, unsigned int width, unsigned int height, const std::string &title, sf::Color background);

    Window(const Window &) = delete;
    Window &operator=(const Window &) = delete;
    Window(Window &&) = delete;
    Window &operator=(Window &&) = delete;

    virtual ~Window() = default;

    void run();
};
