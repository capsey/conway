#pragma once

#include "logger.hpp"

#include <SFML/Graphics.hpp>
#include <atomic>
#include <boost/unordered/unordered_flat_map.hpp>
#include <functional>
#include <memory>

namespace boost
{
    template <>
    struct hash<sf::Vector2i>
    {
        std::size_t operator()(const sf::Vector2i &v) const
        {
            static_assert(sizeof(v.x) == 4 && sizeof(v.y) == 4 && sizeof(size_t) == 8);
            return ((uint64_t)v.y << 32) | (uint32_t)v.x;
        }
    };
}

class Chunk
{
private:
    uint64_t m_data;

public:
    Chunk() : m_data(0) {}
    explicit Chunk(uint64_t data) : m_data(data) {}

    inline uint64_t data() const
    {
        return m_data;
    }

    inline Chunk &set(sf::Vector2i pos, bool state);
    inline bool get(sf::Vector2i pos) const;

    inline Chunk shiftLeft() const
    {
        return Chunk((m_data >> 1) & 0x7F7F7F7F7F7F7F7FULL);
    }

    inline Chunk shiftLeft(unsigned int n) const
    {
        Chunk result = *this;

        while (n--)
            result = result.shiftLeft();

        return result;
    }

    inline Chunk shiftRight() const
    {
        return Chunk((m_data << 1) & 0xFEFEFEFEFEFEFEFEULL);
    }

    inline Chunk shiftRight(unsigned int n) const
    {
        Chunk result = *this;

        while (n--)
            result = result.shiftRight();

        return result;
    }

    inline Chunk shiftUp() const
    {
        return Chunk(m_data >> 8);
    }

    inline Chunk shiftUp(unsigned int n) const
    {
        Chunk result = *this;

        while (n--)
            result = result.shiftUp();

        return result;
    }

    inline Chunk shiftDown() const
    {
        return Chunk(m_data << 8);
    }

    inline Chunk shiftDown(unsigned int n) const
    {
        Chunk result = *this;

        while (n--)
            result = result.shiftDown();

        return result;
    }

    inline explicit operator bool() const
    {
        return m_data;
    }

    inline bool operator==(const Chunk &rhs) const
    {
        return m_data == rhs.m_data;
    }

    inline Chunk &operator|=(const Chunk &other)
    {
        m_data |= other.m_data;
        return *this;
    }

    inline Chunk &operator&=(const Chunk &other)
    {
        m_data &= other.m_data;
        return *this;
    }

    inline Chunk &operator^=(const Chunk &other)
    {
        m_data ^= other.m_data;
        return *this;
    }

    inline Chunk operator~() const
    {
        return Chunk(~m_data);
    }

    inline friend Chunk operator|(Chunk lhs, const Chunk &rhs)
    {
        lhs |= rhs;
        return lhs;
    }

    inline friend Chunk operator&(Chunk lhs, const Chunk &rhs)
    {
        lhs &= rhs;
        return lhs;
    }

    inline friend Chunk operator^(Chunk lhs, const Chunk &rhs)
    {
        lhs ^= rhs;
        return lhs;
    }
};

class BitBoard
{
private:
    boost::unordered_flat_map<sf::Vector2i, Chunk> m_chunks;

public:
    BitBoard &set(sf::Vector2i pos, bool state);
    bool get(sf::Vector2i pos) const;

    inline boost::unordered_flat_map<sf::Vector2i, Chunk>::iterator find(const sf::Vector2i &pos)
    {
        return m_chunks.find(pos);
    }

    inline boost::unordered_flat_map<sf::Vector2i, Chunk>::const_iterator find(const sf::Vector2i &pos) const
    {
        return m_chunks.find(pos);
    }

    inline bool contains(const sf::Vector2i &pos) const
    {
        return m_chunks.contains(pos);
    }

    inline boost::unordered_flat_map<sf::Vector2i, Chunk>::iterator erase(boost::unordered_flat_map<sf::Vector2i, Chunk>::iterator it)
    {
        return m_chunks.erase(it);
    }

    inline boost::unordered_flat_map<sf::Vector2i, Chunk>::size_type erase(const sf::Vector2i &pos)
    {
        return m_chunks.erase(pos);
    }

    inline boost::unordered_flat_map<sf::Vector2i, Chunk>::iterator begin() noexcept
    {
        return m_chunks.begin();
    }

    inline boost::unordered_flat_map<sf::Vector2i, Chunk>::const_iterator begin() const noexcept
    {
        return m_chunks.begin();
    }

    inline boost::unordered_flat_map<sf::Vector2i, Chunk>::iterator end() noexcept
    {
        return m_chunks.end();
    }

    inline boost::unordered_flat_map<sf::Vector2i, Chunk>::const_iterator end() const noexcept
    {
        return m_chunks.end();
    }

    inline void clear()
    {
        m_chunks.clear();
    }

    inline Chunk &operator[](const sf::Vector2i &pos)
    {
        return m_chunks[pos];
    }

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
private:
    BitBoard m_board;
    unsigned int m_ticks = 0;

public:
    const BitBoard &board() const
    {
        return m_board;
    }

    LifeBoard &tick(const LifeBoard &previous);

    LifeBoard &set(sf::Vector2i pos, bool state)
    {
        m_board.set(pos, state);
        return *this;
    }

    bool get(sf::Vector2i pos) const
    {
        return m_board.get(pos);
    }

    unsigned int ticks() const
    {
        return m_ticks;
    }

    LifeBoard &operator|=(const BitBoard &other)
    {
        m_board |= other;
        return *this;
    }

    LifeBoard &operator-=(const BitBoard &other)
    {
        m_board -= other;
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

class LifeBoardRenderer : public sf::Transformable, public sf::Drawable
{
private:
    const LifeBoard &m_data;
    sf::Color m_color;

public:
    LifeBoardRenderer(const LifeBoard &data, sf::Color color) : m_data(data), m_color(color) {}

    void draw(sf::RenderTarget &target, sf::RenderStates states) const override;
};
