#pragma once

#include <SFML/Graphics.hpp>
#include <atomic>
#include <functional>
#include <memory>
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

template <typename T>
class Container
{
private:
    std::atomic<std::shared_ptr<const T>> data;

public:
    Container() : data(std::make_shared<const T>()) {}
    Container(T value) : data(std::make_shared<const T>(std::move(value))) {}

    std::shared_ptr<const T> getData()
    {
        return data.load();
    }

    void modify(std::function<T(const T &)> func)
    {
        while (true)
        {
            auto expected = data.load();
            auto modified = std::make_shared<const T>(func(*expected));

            if (data.compare_exchange_strong(expected, modified))
                break;
        }
    }

    void reset()
    {
        data.store(std::make_shared<const T>());
    }
};

class BitBoard
{
public:
    std::unordered_map<sf::Vector2i, uint64_t> chunks;

    BitBoard set(sf::Vector2i pos, bool state) const;
    bool get(sf::Vector2i pos) const;

    BitBoard &operator|=(const BitBoard &other);
    BitBoard &operator-=(const BitBoard &other);

    friend BitBoard operator|(BitBoard lhs, const BitBoard &rhs)
    {
        lhs |= rhs;
        return lhs;
    }

    friend BitBoard operator-(BitBoard lhs, const BitBoard &rhs)
    {
        lhs -= rhs;
        return lhs;
    }
};

class LifeBoard
{
public:
    BitBoard board;
    unsigned int ticks = 0;

    LifeBoard tick() const;

    LifeBoard &operator|=(const BitBoard &other)
    {
        board |= other;
        return *this;
    }

    LifeBoard &operator-=(const BitBoard &other)
    {
        board -= other;
        return *this;
    }

    friend LifeBoard operator|(LifeBoard lhs, const BitBoard &rhs)
    {
        lhs |= rhs;
        return lhs;
    }

    friend LifeBoard operator-(LifeBoard lhs, const BitBoard &rhs)
    {
        lhs -= rhs;
        return lhs;
    }
};

class ChunkRenderer : public sf::Transformable, public sf::Drawable
{
private:
    static const sf::Texture &texture;

    uint64_t data;
    sf::Color color;

public:
    static void initializeSprites();

    ChunkRenderer(uint64_t data, sf::Color color) : data(data), color(color) {}

    void draw(sf::RenderTarget &target, sf::RenderStates states) const override;
};

class BitBoardRenderer : public sf::Transformable, public sf::Drawable
{
private:
    const BitBoard &data;
    sf::Color color;

public:
    BitBoardRenderer(const BitBoard &data, sf::Color color) : data(data), color(color) {}

    void draw(sf::RenderTarget &target, sf::RenderStates states) const override;
};

class LifeBoardRenderer : public sf::Transformable, public sf::Drawable
{
private:
    const LifeBoard &data;
    sf::Color color;

public:
    LifeBoardRenderer(const LifeBoard &data, sf::Color color) : data(data), color(color) {}

    void draw(sf::RenderTarget &target, sf::RenderStates states) const override;
};
