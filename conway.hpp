#pragma once

#include <SFML/Graphics.hpp>
#include <atomic>
#include <iostream>
#include <shared_mutex>
#include <unordered_map>
#include <unordered_set>

namespace std
{
    template <>
    struct hash<sf::Vector2i>
    {
        std::size_t operator()(const sf::Vector2i &v) const
        {
            size_t h1 = std::hash<int>()(v.x);
            size_t h2 = std::hash<int>()(v.y);
            return h1 ^ (h2 + 0x9e3779b97f4a7c15ULL + (h1 << 6) + (h1 >> 2));
        }
    };
}

class BitBoard : public sf::Transformable, public sf::Drawable
{
protected:
    std::atomic<std::shared_ptr<const std::unordered_map<sf::Vector2i, uint64_t>>> chunks;

public:
    sf::Color color;

    BitBoard(sf::Color color = sf::Color::White) : chunks(std::make_shared<const std::unordered_map<sf::Vector2i, uint64_t>>()), color(color) {}

    void draw(sf::RenderTarget &target, sf::RenderStates states) const override;

    void set(sf::Vector2i pos, bool state);
    bool get(sf::Vector2i pos) const;
    void clear();

    BitBoard &operator|=(const BitBoard &other);
    BitBoard &operator-=(const BitBoard &other);
};

class LifeBoard : public BitBoard
{
public:
    LifeBoard(sf::Color color = sf::Color::White) : BitBoard(color) {}

    void tick();
};

class ChunkRenderer : public sf::Transformable, public sf::Drawable
{
private:
    static const sf::Texture &texture;

public:
    static void initializeSprites();

    uint64_t data;
    sf::Color color;

    ChunkRenderer(uint64_t data, sf::Color color) : data(data), color(color) {}

    void draw(sf::RenderTarget &target, sf::RenderStates states) const override;
};
