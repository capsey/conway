#include "ChunkRenderer.hpp"
#include "Logger.hpp"

#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/Transform.hpp>
#include <SFML/System/Vector2.hpp>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace
{
    sf::Texture texture; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
}

const sf::Texture &ChunkRenderer::m_texture = texture;

void ChunkRenderer::initializeSprites(Logger &logger)
{
    if (!texture.resize(sf::Vector2u(8, 256)))
        throw std::runtime_error("Failed to resize the texture.");

    auto [width, height] = texture.getSize();
    logger.debug("Texture resized successfully to {}x{}.", width, height);

    std::vector<uint8_t> pixels(static_cast<size_t>(width * height * 4));
    logger.debug("Allocated pixel buffer with {} bytes.", pixels.size());

    for (unsigned int i = 0; i < height; i++)
    {
        unsigned int n = i;

        for (unsigned int j = 0; j < width; j++)
        {
            const uint8_t value = (n & 1) ? 255 : 0;
            const size_t index = static_cast<size_t>((i * width) + j) * 4;

            pixels[index + 0] = value;
            pixels[index + 1] = value;
            pixels[index + 2] = value;
            pixels[index + 3] = value;

            n >>= 1;
        }
    }

    texture.update(pixels.data());
    logger.info("Sprite texture initialization completed successfully.");
}

void ChunkRenderer::draw(sf::RenderTarget &target, sf::RenderStates states) const
{
    states.transform *= getTransform();

    for (int i = 0; i < 8; i++)
    {
        sf::Sprite sprite(texture, sf::IntRect({0, static_cast<int>((m_data.data() >> (8 * i)) % 256)}, {8, 1}));
        sprite.setColor(m_color);
        target.draw(sprite, states);

        states.transform = states.transform.translate({0, 1});
    }
}
