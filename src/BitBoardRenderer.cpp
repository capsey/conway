#include "BitBoardRenderer.hpp"
#include "ChunkRenderer.hpp"

#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Transform.hpp>
#include <SFML/System/Vector2.hpp>

void BitBoardRenderer::draw(sf::RenderTarget &target, sf::RenderStates states) const
{
    states.transform *= getTransform();

    for (const auto &[node, meta] : m_data)
    {
        ChunkRenderer chunk(node.chunk, m_color);
        chunk.setPosition(static_cast<sf::Vector2f>(meta.pos * 8));
        target.draw(chunk, states);
    }
}
