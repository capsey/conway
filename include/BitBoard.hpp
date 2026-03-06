#pragma once

#include "Chunk.hpp"
#include "Direction.hpp"
#include "utility.hpp"

#include <SFML/System/Vector2.hpp>
#include <array>
#include <bitset>
#include <boost/container_hash/hash.hpp>
#include <boost/core/bit.hpp>
#include <boost/unordered/unordered_flat_map.hpp>
#include <cassert>
#include <cstddef>
#include <iterator>
#include <limits>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace boost
{
    template <>
    struct hash<sf::Vector2i>
    {
        std::size_t operator()(const sf::Vector2i &v) const
        {
            static_assert(sizeof(v.x) == 4 && sizeof(v.y) == 4 && sizeof(size_t) == 8);
            return (static_cast<uint64_t>(v.y) << 32) | static_cast<uint32_t>(v.x);
        }
    };
}

class BitBoard
{
public:
    using BitPos = sf::Vector2i;
    using ChunkPos = sf::Vector2i;
    using Index = std::size_t;
    using Generation = unsigned int;

    static constexpr Index Invalid = std::numeric_limits<Index>::max();

    struct Node
    {
        Chunk chunk;
        Generation generation = 0;

        constexpr Node(Chunk chunk, Generation generation) : chunk(chunk), generation(generation) {}
    };

    struct Meta
    {
        Index index;
        ChunkPos pos;
        std::array<Index, 8> neighbors = {Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid};

        constexpr Meta(Index index, ChunkPos pos) : index(index), pos(pos) {}
    };

private:
    std::vector<Node> m_nodes;
    std::vector<Meta> m_metas;
    boost::unordered::unordered_flat_map<ChunkPos, Index> m_map;
    Generation m_generation;
    Index m_firstReusable;
    size_t m_size;

    Index allocate(Chunk chunk, ChunkPos pos)
    {
        Index index; // NOLINT(cppcoreguidelines-init-variables)

        for (index = m_firstReusable; index < m_nodes.size(); index++)
            if (m_nodes[index].generation != m_generation)
                break;

        m_firstReusable = index + 1;

        if (index < m_nodes.size())
        {
            m_nodes[index].chunk = chunk;
            m_nodes[index].generation = m_generation;
            disconnect(index);
            connect(index, pos);
        }
        else
        {
            index = m_nodes.size();
            m_nodes.emplace_back(chunk, m_generation);
            m_metas.emplace_back(index, pos);
            connect(index, pos);
        }

        m_size++;
        return index;
    }

    // NOLINTNEXTLINE(misc-no-recursion)
    void connect(Index index, Direction direction, Index other, std::bitset<8> &assigned)
    {
        if (assigned[direction])
            return;

        assigned[direction] = true;
        m_metas[index].neighbors[direction] = other; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)

        if (other == Invalid)
            return;

        Meta &otherMeta = m_metas[other];
        otherMeta.neighbors[direction.opposite()] = index; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)

        for (auto [otherDirection, neighborDirection] : direction.indirects())
            connect(index, neighborDirection, otherMeta.neighbors[otherDirection], assigned); // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
    }

    void connect(Index index, ChunkPos pos)
    {
        Meta &meta = m_metas[index];
        meta = Meta(index, pos);

        std::bitset<8> assigned;

        for (auto direction : Direction::All)
        {
            if (assigned[direction])
                continue;

            auto entry = m_map.find(pos + direction.offset());
            connect(index, direction, entry != m_map.end() ? entry->second : Invalid, assigned);
        }

        m_map[pos] = index;
    }

    void disconnect(Index index)
    {
        Meta &meta = m_metas[index];

        for (auto direction : Direction::All)
            if (meta.neighbors[direction] != Invalid)                                         // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
                m_metas[meta.neighbors[direction]].neighbors[direction.opposite()] = Invalid; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)

        m_map.erase(meta.pos);
    }

public:
    class const_iterator
    {
    private:
        const std::vector<Node> *m_nodes;
        const std::vector<Meta> *m_metas;
        Generation m_generation;
        Index m_index;

        constexpr void skipStale()
        {
            assert(m_nodes && m_metas);
            while (m_index < m_nodes->size() && (*m_nodes)[m_index].generation != m_generation)
                m_index++;
        }

    public:
        struct reference
        {
            const Node &node; // NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members)
            const Meta &meta; // NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members)

            constexpr reference(const Node &node, const Meta &meta) : node(node), meta(meta) {}

            constexpr reference *operator->() noexcept
            {
                return this;
            }
        };

        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = reference;
        using pointer = reference *;

        constexpr const_iterator(const std::vector<Node> *nodes, const std::vector<Meta> *metas, Generation generation, Index index) : m_nodes(nodes), m_metas(metas), m_generation(generation), m_index(index)
        {
            assert(nodes->size() == metas->size());
            skipStale();
        }

        constexpr reference operator*() const
        {
            assert(m_nodes && m_metas && m_index < m_nodes->size() && m_index < m_metas->size());
            return {(*m_nodes)[m_index], (*m_metas)[m_index]};
        }

        constexpr reference operator->() const
        {
            assert(m_nodes && m_metas && m_index < m_nodes->size() && m_index < m_metas->size());
            return {(*m_nodes)[m_index], (*m_metas)[m_index]};
        }

        constexpr const_iterator &operator++()
        {
            m_index++;
            skipStale();
            return *this;
        }

        constexpr const_iterator operator++(int)
        {
            const_iterator result = *this;
            m_index++;
            skipStale();
            return result;
        }

        constexpr bool operator==(const const_iterator &other) const
        {
            return m_nodes == other.m_nodes && m_metas == other.m_metas && m_generation == other.m_generation && m_index == other.m_index;
        }

        constexpr bool operator!=(const const_iterator &other) const
        {
            return !(*this == other);
        }
    };

    BitBoard() : m_generation(1), m_firstReusable(0), m_size(0) {}
    BitBoard(Generation generation) : m_generation(generation), m_firstReusable(0), m_size(0) {}

    BitBoard &set(BitPos pos, bool state)
    {
        ChunkPos chunkPos = utility::floorDiv(pos, {8, 8});
        BitPos localPos = pos - (chunkPos * 8);

        if (auto entry = m_map.find(chunkPos); entry != m_map.end())
        {
            Node &node = m_nodes[entry->second];

            if (node.generation == m_generation)
            {
                if (!node.chunk.set(localPos, state))
                {
                    m_size--;
                    node.generation = 0;
                }
            }
            else if (state)
            {
                m_size++;
                node.chunk = Chunk().set(localPos, state);
                node.generation = m_generation;
            }
        }
        else if (state)
        {
            allocate(Chunk().set(localPos, state), chunkPos);
        }

        return *this;
    }

    [[nodiscard]] bool get(BitPos pos) const
    {
        ChunkPos chunkPos = utility::floorDiv(pos, {8, 8});
        BitPos localPos = pos - (chunkPos * 8);

        if (auto entry = m_map.find(chunkPos); entry != m_map.end())
            return m_nodes[entry->second].chunk.get(localPos);

        return false;
    }

    [[nodiscard]] constexpr const_iterator begin() const
    {
        return {&m_nodes, &m_metas, m_generation, 0};
    }

    [[nodiscard]] constexpr const_iterator end() const
    {
        return {&m_nodes, &m_metas, m_generation, m_nodes.size()};
    }

    [[nodiscard]] const_iterator find(ChunkPos pos) const
    {
        if (auto entry = m_map.find(pos); entry != m_map.end())
        {
            if (m_nodes[entry->second].generation != m_generation)
                return end();

            return {&m_nodes, &m_metas, m_generation, entry->second};
        }

        return end();
    }

    BitBoard &store(ChunkPos pos, Chunk chunk)
    {
        if (auto entry = m_map.find(pos); entry != m_map.end())
        {
            Node &node = m_nodes[entry->second];

            if (chunk)
            {
                if (node.generation != m_generation)
                    m_size++;

                node.chunk = chunk;
                node.generation = m_generation;
            }
            else
            {
                if (node.generation == m_generation)
                    m_size--;

                node.generation = 0;
            }
        }
        else if (chunk)
        {
            allocate(chunk, pos);
        }

        return *this;
    }

    [[nodiscard]] constexpr Generation getGeneration() const
    {
        return m_generation;
    }

    constexpr void setGeneration(Generation generation)
    {
        assert(generation > m_generation);
        m_generation = generation;
        m_firstReusable = 0;
        m_size = 0;
    }

    void clear()
    {
        m_nodes.clear();
        m_metas.clear();
        m_map.clear();
        m_generation = 1;
        m_firstReusable = 0;
        m_size = 0;
    }

    [[nodiscard]] constexpr const_iterator at(Index index) const
    {
        if (index == Invalid)
            return end();

        assert(index < m_nodes.size());

        if (m_nodes[index].generation != m_generation)
            return end();

        return {&m_nodes, &m_metas, m_generation, index};
    }

    [[nodiscard]] constexpr size_t size() const
    {
        return m_size;
    }

    BitBoard &operator|=(const BitBoard &other)
    {
        for (const auto &[otherNode, otherMeta] : other)
        {
            if (auto entry = m_map.find(otherMeta.pos); entry != m_map.end())
            {
                Node &node = m_nodes[entry->second];

                if (node.generation == m_generation)
                    node.chunk |= otherNode.chunk;
                else
                    node.chunk = otherNode.chunk;

                node.generation = m_generation;
            }
            else
            {
                allocate(otherNode.chunk, otherMeta.pos);
            }
        }

        return *this;
    }

    BitBoard &operator-=(const BitBoard &other)
    {
        for (const auto &[otherNode, otherMeta] : other)
        {
            if (auto entry = m_map.find(otherMeta.pos); entry != m_map.end())
            {
                Node &node = m_nodes[entry->second];

                if (node.generation == m_generation)
                {
                    node.chunk -= otherNode.chunk;

                    if (!node.chunk)
                    {
                        node.generation = 0;
                        m_size--;
                    }
                }
            }
        }

        return *this;
    }

    [[nodiscard]] friend BitBoard operator|(BitBoard lhs, const BitBoard &rhs)
    {
        lhs |= rhs;
        return lhs;
    }

    [[nodiscard]] friend BitBoard operator-(BitBoard lhs, const BitBoard &rhs)
    {
        lhs -= rhs;
        return lhs;
    }
};

template <>
struct std::tuple_size<BitBoard::const_iterator::reference> : std::integral_constant<std::size_t, 2>
{
};

template <std::size_t I>
struct std::tuple_element<I, BitBoard::const_iterator::reference>
{
    using type = std::tuple_element_t<I, std::tuple<const BitBoard::Node &, const BitBoard::Meta &>>;
};

template <std::size_t I>
constexpr decltype(auto) get(BitBoard::const_iterator::reference proxy) noexcept
{
    if constexpr (I == 0)
        return (proxy.node);
    else if constexpr (I == 1)
        return (proxy.meta);
}
