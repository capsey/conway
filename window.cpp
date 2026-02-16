#include "window.hpp"

#include <algorithm>
#include <cmath>

Window::Window(Logger &logger, unsigned int width, unsigned int height, std::string title, sf::Color background) : logger(logger), window(sf::VideoMode({width, height}), title), view({0.0F, 0.0F}, {(float)width, (float)height}), background(background)
{
    window.setVerticalSyncEnabled(true);

    addEventHandler<sf::Event::Closed>([&](const sf::Event::Closed &event)
    {
        window.close();
    });

    addEventHandler<sf::Event::Resized>([&](const sf::Event::Resized &event)
    {
        m_view.setSize(sf::Vector2f(event.size));
    });

    addEventHandler<sf::Event::MouseWheelScrolled>([&](const sf::Event::MouseWheelScrolled &event)
    {
        if (event.wheel == sf::Mouse::Wheel::Vertical)
        {
            float newZoom = std::clamp(m_zoom * std::powf(0.5F, event.delta), 1.0F / 64.0F, 1.0F);
            sf::Vector2f originDrift = window.mapPixelToCoords(event.position) - window.getView().getCenter();
            m_view.move((1 - (newZoom / m_zoom)) * originDrift);
            m_zoom = newZoom;
        }
    });
}

void Window::run()
{
    try
    {
        logger.info("Initializing the window...");
        initialize();

        logger.info("The window event loop started.");

        while (window.isOpen())
        {
            while (const std::optional event = window.pollEvent())
                for (auto &handler : m_handlers)
                    handler(*event);

            view = m_view;
            view.zoom(m_zoom);

            mousePos = sf::Mouse::getPosition(window);
            worldPos = window.mapPixelToCoords(mousePos);

            if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Middle))
                m_view.move(window.mapPixelToCoords(prevMousePos) - worldPos);

            update();

            prevMousePos = mousePos;
            prevWorldPos = worldPos;

            window.clear(background);
            window.setView(view);

            draw();

            window.display();
        }

        logger.info("Deinitializing the window...");
        deinitialize();
    }
    catch (const std::exception &e)
    {
        logger.error(e.what());
    }
}
