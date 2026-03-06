#pragma once

#include <SFML/System/Vector2.hpp>
#include <cassert>
#include <cstdint>

class Chunk
{
private:
    uint64_t m_data;

public:
    constexpr Chunk() : m_data(0) {}
    explicit constexpr Chunk(uint64_t data) : m_data(data) {}

    [[nodiscard]] constexpr uint64_t data() const
    {
        return m_data;
    }

    constexpr Chunk &set(sf::Vector2i pos, bool state)
    {
        int i = (pos.y * 8) + pos.x;
        assert(i >= 0 && i < 64);
        uint64_t mask = 1ULL << i;
        m_data = state ? m_data | mask : m_data & ~mask;
        return *this;
    }

    [[nodiscard]] constexpr bool get(sf::Vector2i pos) const
    {
        int i = (pos.y * 8) + pos.x;
        assert(i >= 0 && i < 64);
        return (m_data >> i) & 1;
    }

    [[nodiscard]] constexpr Chunk shiftLeft() const
    {
        return Chunk((m_data >> 1) & 0x7F7F7F7F7F7F7F7FULL);
    }

    [[nodiscard]] constexpr Chunk shiftLeft(unsigned int n) const
    {
        Chunk result = *this;

        while (n--)
            result = result.shiftLeft();

        return result;
    }

    [[nodiscard]] constexpr Chunk shiftRight() const
    {
        return Chunk((m_data << 1) & 0xFEFEFEFEFEFEFEFEULL);
    }

    [[nodiscard]] constexpr Chunk shiftRight(unsigned int n) const
    {
        Chunk result = *this;

        while (n--)
            result = result.shiftRight();

        return result;
    }

    [[nodiscard]] constexpr Chunk shiftUp() const
    {
        return Chunk(m_data >> 8);
    }

    [[nodiscard]] constexpr Chunk shiftUp(unsigned int n) const
    {
        Chunk result = *this;

        while (n--)
            result = result.shiftUp();

        return result;
    }

    [[nodiscard]] constexpr Chunk shiftDown() const
    {
        return Chunk(m_data << 8);
    }

    [[nodiscard]] constexpr Chunk shiftDown(unsigned int n) const
    {
        Chunk result = *this;

        while (n--)
            result = result.shiftDown();

        return result;
    }

    constexpr explicit operator bool() const
    {
        return m_data;
    }

    constexpr bool operator==(const Chunk &rhs) const
    {
        return m_data == rhs.m_data;
    }

    constexpr Chunk &operator|=(const Chunk &other)
    {
        m_data |= other.m_data;
        return *this;
    }

    constexpr Chunk &operator&=(const Chunk &other)
    {
        m_data &= other.m_data;
        return *this;
    }

    constexpr Chunk &operator^=(const Chunk &other)
    {
        m_data ^= other.m_data;
        return *this;
    }

    constexpr Chunk &operator-=(const Chunk &other)
    {
        m_data &= ~other.m_data;
        return *this;
    }

    [[nodiscard]] constexpr Chunk operator~() const
    {
        return Chunk(~m_data);
    }

    [[nodiscard]] constexpr friend Chunk operator|(Chunk lhs, const Chunk &rhs)
    {
        lhs |= rhs;
        return lhs;
    }

    [[nodiscard]] constexpr friend Chunk operator&(Chunk lhs, const Chunk &rhs)
    {
        lhs &= rhs;
        return lhs;
    }

    [[nodiscard]] constexpr friend Chunk operator^(Chunk lhs, const Chunk &rhs)
    {
        lhs ^= rhs;
        return lhs;
    }

    [[nodiscard]] constexpr friend Chunk operator-(Chunk lhs, const Chunk &rhs)
    {
        lhs -= rhs;
        return lhs;
    }
};
