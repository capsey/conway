#pragma once

#include <SFML/Graphics.hpp>
#include <functional>

class Window
{
private:
    std::vector<std::function<void(const sf::Event &)>> m_handlers;

    sf::View m_view;
    float m_zoom = 1.0F / 4.0F;

protected:
    sf::RenderWindow window;
    sf::View view;
    sf::Color background = sf::Color::Black;

    sf::Vector2i mousePos;
    sf::Vector2i prevMousePos;
    sf::Vector2f worldPos;
    sf::Vector2f prevWorldPos;

    template <typename T>
    void addEventHandler(std::function<void(const T &)> handler)
    {
        m_handlers.push_back([=](const sf::Event &event)
        {
            if (const auto *t = event.getIf<T>())
                handler(*t);
        });
    }

    virtual void update() = 0;
    virtual void draw() = 0;

public:
    Window(unsigned int width, unsigned int height, std::string title, sf::Color background);
    ~Window() {}

    void run();
};
