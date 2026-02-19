#pragma once

#include "logger.hpp"

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
    std::atomic<std::shared_ptr<const T>> m_data;

public:
    Container() : m_data(std::make_shared<const T>()) {}
    Container(T data) : m_data(std::make_shared<const T>(std::move(data))) {}

    std::shared_ptr<const T> get()
    {
        return m_data.load();
    }

    void modify(std::function<T(const T &)> func)
    {
        while (true)
        {
            auto expected = m_data.load();
            auto modified = std::make_shared<const T>(func(*expected));

            if (m_data.compare_exchange_strong(expected, modified))
                break;
        }
    }

    void reset()
    {
        m_data.store(std::make_shared<const T>());
    }
};

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
            result = std::move(result.shiftLeft());

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
            result = std::move(result.shiftRight());

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
            result = std::move(result.shiftUp());

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
            result = std::move(result.shiftDown());

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
    std::unordered_map<sf::Vector2i, Chunk> m_chunks;

public:
    void set(sf::Vector2i pos, bool state);
    bool get(sf::Vector2i pos) const;

    std::unordered_map<sf::Vector2i, Chunk>::iterator find(const sf::Vector2i &pos)
    {
        return m_chunks.find(pos);
    }

    std::unordered_map<sf::Vector2i, Chunk>::const_iterator find(const sf::Vector2i &pos) const
    {
        return m_chunks.find(pos);
    }

    std::unordered_map<sf::Vector2i, Chunk>::iterator erase(std::unordered_map<sf::Vector2i, Chunk>::iterator it)
    {
        return m_chunks.erase(it);
    }

    std::unordered_map<sf::Vector2i, Chunk>::size_type erase(const sf::Vector2i &pos)
    {
        return m_chunks.erase(pos);
    }

    std::unordered_map<sf::Vector2i, Chunk>::iterator begin() noexcept
    {
        return m_chunks.begin();
    }

    std::unordered_map<sf::Vector2i, Chunk>::const_iterator begin() const noexcept
    {
        return m_chunks.begin();
    }

    std::unordered_map<sf::Vector2i, Chunk>::iterator end() noexcept
    {
        return m_chunks.end();
    }

    std::unordered_map<sf::Vector2i, Chunk>::const_iterator end() const noexcept
    {
        return m_chunks.end();
    }

    Chunk &operator[](const sf::Vector2i &pos)
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

    LifeBoard next() const;

    void set(sf::Vector2i pos, bool state)
    {
        m_board.set(pos, state);
    }

    bool get(sf::Vector2i pos) const
    {
        return m_board.get(pos);
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
