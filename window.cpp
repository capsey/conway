#include "window.hpp"

#include <algorithm>
#include <cmath>

Window::Window(Logger &logger, unsigned int width, unsigned int height, std::string title, sf::Color background) : logger(logger), window(sf::VideoMode({width, height}), title), size(width, height), view({0.0F, 0.0F}, {(float)width, (float)height}), background(background)
{
    window.setVerticalSyncEnabled(true);

    addEventHandler<sf::Event::Closed>([&](const sf::Event::Closed &)
    {
        window.close();
    });

    addEventHandler<sf::Event::Resized>([&](const sf::Event::Resized &event)
    {
        float zoom = view.getSize().x / (float)size.x;

        size = event.size;
        view.setSize(sf::Vector2f(size));
        view.zoom(zoom);
    });

    addEventHandler<sf::Event::MouseMoved>([&](const sf::Event::MouseMoved &event)
    {
        prevMousePos = mousePos;
        prevWorldPos = worldPos;

        mousePos = event.position;
        worldPos = window.mapPixelToCoords(mousePos);

        if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Middle))
            view.move(window.mapPixelToCoords(prevMousePos) - worldPos);
    });

    addEventHandler<sf::Event::MouseWheelScrolled>([&](const sf::Event::MouseWheelScrolled &event)
    {
        if (event.wheel == sf::Mouse::Wheel::Vertical)
        {
            float zoom = view.getSize().x / (float)size.x;
            float newZoom = std::clamp(zoom * std::powf(0.5F, event.delta), 1.0F / 64.0F, 1.0F);
            sf::Vector2f originDrift = window.mapPixelToCoords(event.position) - window.getView().getCenter();
            view.move((1 - (newZoom / zoom)) * originDrift);
            view.zoom(newZoom / zoom);
        }
    });
}

void Window::run()
{
    logger.info("Initializing the window...");
    initialize();

    try
    {
        logger.info("The window event loop started.");

        while (window.isOpen())
        {
            while (const std::optional event = window.pollEvent())
                for (auto &handler : m_handlers)
                    handler(*event);

            update();

            window.clear(background);
            window.setView(view);

            draw();

            window.display();
        }
    }
    catch (const std::exception &e)
    {
        logger.error(e.what());
    }

    logger.info("Deinitializing the window...");
    deinitialize();
}
