#pragma once

#include "bitboard.hpp"
#include "logger.hpp"

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Drawable.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/Transformable.hpp>

void tick(const BitBoard &previous, BitBoard &current);

class ChunkRenderer : public sf::Transformable, public sf::Drawable
{
private:
    static const sf::Texture &m_texture;

    Chunk m_data;
    sf::Color m_color;

public:
    static void initializeSprites(Logger &logger);

    ChunkRenderer(Chunk data, sf::Color color) : m_data(data), m_color(color) {}

    void draw(sf::RenderTarget &target, sf::RenderStates states) const override;
};

class BitBoardRenderer : public sf::Transformable, public sf::Drawable
{
private:
    const BitBoard &m_data;
    sf::Color m_color;

public:
    BitBoardRenderer(const BitBoard &data, sf::Color color) : m_data(data), m_color(color) {}

    void draw(sf::RenderTarget &target, sf::RenderStates states) const override;
};
