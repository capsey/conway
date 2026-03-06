#pragma once

#include <SFML/System/Vector2.hpp>
#include <array>
#include <cstdint>
#include <span>
#include <utility>

class Direction
{
private:
    uint8_t m_value;

    constexpr Direction(uint8_t value) : m_value(value) {}

public:
    static const Direction North;
    static const Direction South;
    static const Direction West;
    static const Direction East;
    static const Direction NorthWest;
    static const Direction NorthEast;
    static const Direction SouthWest;
    static const Direction SouthEast;

    static const std::array<Direction, 8> All;

private:
    static const std::array<std::pair<Direction, Direction>, 4> NorthIndirects;
    static const std::array<std::pair<Direction, Direction>, 4> SouthIndirects;
    static const std::array<std::pair<Direction, Direction>, 4> WestIndirects;
    static const std::array<std::pair<Direction, Direction>, 4> EastIndirects;
    static const std::array<std::pair<Direction, Direction>, 2> NorthWestIndirects;
    static const std::array<std::pair<Direction, Direction>, 2> NorthEastIndirects;
    static const std::array<std::pair<Direction, Direction>, 2> SouthWestIndirects;
    static const std::array<std::pair<Direction, Direction>, 2> SouthEastIndirects;

    static const std::array<Direction, 8> Opposites;
    static const std::array<sf::Vector2i, 8> Offsets;
    static const std::array<std::span<const std::pair<Direction, Direction>>, 8> Indirects;

public:
    [[nodiscard]] constexpr Direction opposite() const;
    [[nodiscard]] constexpr std::span<const std::pair<Direction, Direction>> indirects() const;
    [[nodiscard]] constexpr sf::Vector2i offset(sf::Vector2i pos = {0, 0}) const;

    constexpr operator uint8_t() const { return m_value; }
};

inline constexpr Direction Direction::North = 0;
inline constexpr Direction Direction::South = 1;
inline constexpr Direction Direction::West = 2;
inline constexpr Direction Direction::East = 3;
inline constexpr Direction Direction::NorthWest = 4;
inline constexpr Direction Direction::NorthEast = 5;
inline constexpr Direction Direction::SouthWest = 6;
inline constexpr Direction Direction::SouthEast = 7;

inline constexpr std::array<Direction, 8> Direction::All = {{North, South, West, East, NorthWest, NorthEast, SouthWest, SouthEast}};

inline constexpr std::array<std::pair<Direction, Direction>, 4> Direction::NorthIndirects = {{{West, NorthWest}, {East, NorthEast}, {SouthWest, West}, {SouthEast, East}}};
inline constexpr std::array<std::pair<Direction, Direction>, 4> Direction::SouthIndirects = {{{West, SouthWest}, {East, SouthEast}, {NorthWest, West}, {NorthEast, East}}};
inline constexpr std::array<std::pair<Direction, Direction>, 4> Direction::WestIndirects = {{{North, NorthWest}, {South, SouthWest}, {NorthEast, North}, {SouthEast, South}}};
inline constexpr std::array<std::pair<Direction, Direction>, 4> Direction::EastIndirects = {{{North, NorthEast}, {South, SouthEast}, {NorthWest, North}, {SouthWest, South}}};
inline constexpr std::array<std::pair<Direction, Direction>, 2> Direction::NorthWestIndirects = {{{South, West}, {East, North}}};
inline constexpr std::array<std::pair<Direction, Direction>, 2> Direction::NorthEastIndirects = {{{South, East}, {West, North}}};
inline constexpr std::array<std::pair<Direction, Direction>, 2> Direction::SouthWestIndirects = {{{North, West}, {East, South}}};
inline constexpr std::array<std::pair<Direction, Direction>, 2> Direction::SouthEastIndirects = {{{North, East}, {West, South}}};

inline constexpr std::array<Direction, 8> Direction::Opposites = {{South, North, East, West, SouthEast, SouthWest, NorthEast, NorthWest}};
inline constexpr std::array<sf::Vector2i, 8> Direction::Offsets = {{{0, -1}, {0, 1}, {-1, 0}, {1, 0}, {-1, -1}, {1, -1}, {-1, 1}, {1, 1}}};
inline constexpr std::array<std::span<const std::pair<Direction, Direction>>, 8> Direction::Indirects = {NorthIndirects, SouthIndirects, WestIndirects, EastIndirects, NorthWestIndirects, NorthEastIndirects, SouthWestIndirects, SouthEastIndirects};

constexpr Direction Direction::opposite() const
{
    return Opposites[m_value]; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
}

constexpr std::span<const std::pair<Direction, Direction>> Direction::indirects() const
{
    return Indirects[m_value]; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
}

constexpr sf::Vector2i Direction::offset(sf::Vector2i pos) const
{
    return pos + Offsets[m_value]; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
}
