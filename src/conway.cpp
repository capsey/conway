#include "conway.hpp"
#include "BitBoard.hpp"
#include "Chunk.hpp"
#include "Direction.hpp"

#include <SFML/System/Vector2.hpp>
#include <boost/unordered/unordered_flat_map.hpp>
#include <tuple>

namespace
{
    [[nodiscard]] constexpr std::tuple<Chunk, Chunk> halfAdder(Chunk a, Chunk b)
    {
        return {a ^ b, a & b};
    }

    [[nodiscard]] constexpr std::tuple<Chunk, Chunk> fullAdder(Chunk a, Chunk b, Chunk c)
    {
        Chunk s = a ^ b;
        return {s ^ c, (a & b) | (s & c)};
    }

    [[nodiscard]] constexpr std::tuple<Chunk, Chunk, Chunk> adder2(Chunk a0, Chunk a1, Chunk b0, Chunk b1)
    {
        auto [s0, c0] = halfAdder(a0, b0);
        auto [s1, c1] = fullAdder(a1, b1, c0);
        return {s0, s1, c1};
    }

    [[nodiscard]] constexpr std::tuple<Chunk, Chunk, Chunk, Chunk> adder3(Chunk a0, Chunk a1, Chunk a2, Chunk b0, Chunk b1, Chunk b2)
    {
        auto [s0, c0] = halfAdder(a0, b0);
        auto [s1, c1] = fullAdder(a1, b1, c0);
        auto [s2, c2] = fullAdder(a2, b2, c1);
        return {s0, s1, s2, c2};
    }

    [[nodiscard]] inline Chunk process(const BitBoard &board, Chunk chunk, const BitBoard::Meta &meta, boost::unordered::unordered_flat_map<sf::Vector2i, BitBoard::Meta> *potentialChunks = nullptr)
    {
        Chunk x0 = chunk.shiftRight(); // left neighbor
        Chunk x1 = chunk.shiftLeft();  // right neighbor
        Chunk x2 = chunk.shiftDown();  // upper neighbor
        Chunk x3 = chunk.shiftUp();    // lower neighbor
        Chunk x4 = x0.shiftDown();     // upper left neighbor
        Chunk x5 = x0.shiftUp();       // lower left neighbor
        Chunk x6 = x1.shiftDown();     // upper right neighbor
        Chunk x7 = x1.shiftUp();       // lower right neighbor

        if (auto other = board.at(meta.neighbors[Direction::North]); other != board.end())
        {
            Chunk y = other->node.chunk.shiftUp(7);
            x2 |= y;
            x6 |= y.shiftLeft();
            x4 |= y.shiftRight();
        }
        else if (potentialChunks && chunk.shiftDown(7))
        {
            Direction direction = Direction::North;
            auto [entry, inserted] = potentialChunks->try_emplace(direction.offset(meta.pos), 0, direction.offset(meta.pos));
            entry->second.neighbors[direction.opposite()] = meta.index; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
        }

        if (auto other = board.at(meta.neighbors[Direction::South]); other != board.end())
        {
            Chunk y = other->node.chunk.shiftDown(7);
            x3 |= y;
            x7 |= y.shiftLeft();
            x5 |= y.shiftRight();
        }
        else if (potentialChunks && chunk.shiftUp(7))
        {
            Direction direction = Direction::South;
            auto [entry, inserted] = potentialChunks->try_emplace(direction.offset(meta.pos), 0, direction.offset(meta.pos));
            entry->second.neighbors[direction.opposite()] = meta.index; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
        }

        if (auto other = board.at(meta.neighbors[Direction::West]); other != board.end())
        {
            Chunk y = other->node.chunk.shiftLeft(7);
            x0 |= y;
            x4 |= y.shiftDown();
            x5 |= y.shiftUp();
        }
        else if (potentialChunks && chunk.shiftRight(7))
        {
            Direction direction = Direction::West;
            auto [entry, inserted] = potentialChunks->try_emplace(direction.offset(meta.pos), 0, direction.offset(meta.pos));
            entry->second.neighbors[direction.opposite()] = meta.index; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
        }

        if (auto other = board.at(meta.neighbors[Direction::East]); other != board.end())
        {
            Chunk y = other->node.chunk.shiftRight(7);
            x1 |= y;
            x6 |= y.shiftDown();
            x7 |= y.shiftUp();
        }
        else if (potentialChunks && chunk.shiftLeft(7))
        {
            Direction direction = Direction::East;
            auto [entry, inserted] = potentialChunks->try_emplace(direction.offset(meta.pos), 0, direction.offset(meta.pos));
            entry->second.neighbors[direction.opposite()] = meta.index; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
        }

        if (auto other = board.at(meta.neighbors[Direction::NorthWest]); other != board.end())
        {
            Chunk y = other->node.chunk.shiftLeft(7).shiftUp(7);
            x4 |= y;
        }
        else if (potentialChunks && chunk.shiftDown(7).shiftRight(7))
        {
            Direction direction = Direction::NorthWest;
            auto [entry, inserted] = potentialChunks->try_emplace(direction.offset(meta.pos), 0, direction.offset(meta.pos));
            entry->second.neighbors[direction.opposite()] = meta.index; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
        }

        if (auto other = board.at(meta.neighbors[Direction::NorthEast]); other != board.end())
        {
            Chunk y = other->node.chunk.shiftRight(7).shiftUp(7);
            x6 |= y;
        }
        else if (potentialChunks && chunk.shiftDown(7).shiftLeft(7))
        {
            Direction direction = Direction::NorthEast;
            auto [entry, inserted] = potentialChunks->try_emplace(direction.offset(meta.pos), 0, direction.offset(meta.pos));
            entry->second.neighbors[direction.opposite()] = meta.index; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
        }

        if (auto other = board.at(meta.neighbors[Direction::SouthWest]); other != board.end())
        {
            Chunk y = other->node.chunk.shiftLeft(7).shiftDown(7);
            x5 |= y;
        }
        else if (potentialChunks && chunk.shiftUp(7).shiftRight(7))
        {
            Direction direction = Direction::SouthWest;
            auto [entry, inserted] = potentialChunks->try_emplace(direction.offset(meta.pos), 0, direction.offset(meta.pos));
            entry->second.neighbors[direction.opposite()] = meta.index; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
        }

        if (auto other = board.at(meta.neighbors[Direction::SouthEast]); other != board.end())
        {
            Chunk y = other->node.chunk.shiftRight(7).shiftDown(7);
            x7 |= y;
        }
        else if (potentialChunks && chunk.shiftUp(7).shiftLeft(7))
        {
            Direction direction = Direction::SouthEast;
            auto [entry, inserted] = potentialChunks->try_emplace(direction.offset(meta.pos), 0, direction.offset(meta.pos));
            entry->second.neighbors[direction.opposite()] = meta.index; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
        }

        auto [s01, c01] = halfAdder(x0, x1);
        auto [s23, c23] = halfAdder(x2, x3);
        auto [s45, c45] = halfAdder(x4, x5);
        auto [s67, c67] = halfAdder(x6, x7);
        auto [q00, q01, c0] = adder2(s01, c01, s23, c23);
        auto [q10, q11, c1] = adder2(s45, c45, s67, c67);
        auto [r0, r1, r2, r3] = adder3(q00, q01, c0, q10, q11, c1);

        return r1 & ~r2 & (r0 | chunk);
    }
}

namespace conway
{
    void tick(const BitBoard &previous, BitBoard &current)
    {
        current.setGeneration(previous.getGeneration() + 1);

        boost::unordered::unordered_flat_map<sf::Vector2i, BitBoard::Meta> potentialChunks;

        for (const auto &[node, meta] : previous)
            if (auto chunk = process(previous, node.chunk, meta, &potentialChunks))
                current.store(meta.pos, chunk);

        for (const auto &[pos, meta] : potentialChunks)
            if (auto chunk = process(previous, Chunk(), meta))
                current.store(pos, chunk);
    }
}
