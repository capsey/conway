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

    inline Chunk &operator-=(const Chunk &other)
    {
        m_data &= ~other.m_data;
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

    inline friend Chunk operator-(Chunk lhs, const Chunk &rhs)
    {
        lhs -= rhs;
        return lhs;
    }
};

class BitBoard
{
public:
    static constexpr size_t Invalid = std::numeric_limits<size_t>::max();

    struct Node
    {
        Chunk chunk;
        unsigned int generation;

        Node() = default;
        Node(Chunk chunk, unsigned int generation) : chunk(chunk), generation(generation) {}
    };

    struct Meta
    {
        size_t n = Invalid;
        size_t s = Invalid;
        size_t w = Invalid;
        size_t e = Invalid;
        size_t nw = Invalid;
        size_t ne = Invalid;
        size_t sw = Invalid;
        size_t se = Invalid;
        sf::Vector2i pos;

        Meta() = default;
        Meta(sf::Vector2i pos) : pos(pos) {}
    };

private:
    template <typename Node, typename Meta>
    struct Proxy
    {
        Node &node;
        Meta &meta;

        Proxy(Node &node, Meta &meta) : node(node), meta(meta) {}

        Proxy *operator->()
        {
            return this;
        }
    };

    template <typename Node, typename Meta>
    class Iterator
    {
    private:
        template <typename, typename>
        friend class Iterator;

        using NodeVector = std::conditional_t<std::is_const_v<Node>, const std::vector<std::remove_const_t<Node>>, std::vector<Node>>;
        using MetaVector = std::conditional_t<std::is_const_v<Meta>, const std::vector<std::remove_const_t<Meta>>, std::vector<Meta>>;

        NodeVector *m_nodes;
        MetaVector *m_metas;
        unsigned int m_generation;
        size_t m_index;

        void skipEmpty()
        {
            assert(m_nodes && m_metas);
            while (m_index < m_nodes->size() && (*m_nodes)[m_index].generation != m_generation)
                m_index++;
        }

    public:
        Iterator(NodeVector *nodes, MetaVector *metas, unsigned int generation, size_t index) : m_nodes(nodes), m_metas(metas), m_generation(generation), m_index(index)
        {
            assert(nodes->size() == metas->size());
            skipEmpty();
        }

        template <typename OtherNode, typename OtherMeta>
            requires std::is_convertible_v<OtherNode *, Node *> && std::is_convertible_v<OtherMeta *, Meta *>
        Iterator(const Iterator<OtherNode, OtherMeta> &other) : Iterator(other.m_nodes, other.m_metas, other.m_generation, other.m_index)
        {
        }

        Proxy<Node, Meta> operator*() const
        {
            assert(m_nodes && m_metas && m_index < m_nodes->size() && m_index < m_metas->size());
            return {(*m_nodes)[m_index], (*m_metas)[m_index]};
        }

        Proxy<Node, Meta> operator->() const
        {
            assert(m_nodes && m_metas && m_index < m_nodes->size() && m_index < m_metas->size());
            return {(*m_nodes)[m_index], (*m_metas)[m_index]};
        }

        Iterator &operator++()
        {
            m_index++;
            skipEmpty();
            return *this;
        }

        Iterator operator++(int)
        {
            Iterator result = *this;
            m_index++;
            skipEmpty();
            return result;
        }

        bool operator==(const Iterator &other) const
        {
            return m_nodes == other.m_nodes && m_metas == other.m_metas && m_generation == other.m_generation && m_index == other.m_index;
        }

        bool operator!=(const Iterator &other) const
        {
            return !(*this == other);
        }
    };

    std::vector<Node> m_nodes;
    std::vector<Meta> m_metas;
    boost::unordered_flat_map<sf::Vector2i, size_t> m_map;
    unsigned int m_generation;
    size_t m_firstReusable;

public:
    using iterator = Iterator<Node, Meta>;
    using const_iterator = Iterator<const Node, const Meta>;

private:
    size_t allocate(Chunk chunk, sf::Vector2i pos)
    {
        size_t index;

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
            m_metas.emplace_back(pos);
            connect(index, pos);
        }

        return index;
    }

    void connect(size_t index, sf::Vector2i pos)
    {
        Meta &meta = m_metas[index];

        boost::unordered_flat_map<sf::Vector2i, size_t>::iterator entry;
        bool n = false;
        bool s = false;
        bool w = false;
        bool e = false;
        bool nw = false;
        bool ne = false;
        bool sw = false;
        bool se = false;

        if (!n && (entry = m_map.find(pos + sf::Vector2i(0, -1))) != m_map.end())
        {
            Meta &otherMeta = m_metas[entry->second];

            meta.n = entry->second;
            otherMeta.s = index;

            if (!nw)
            {
                meta.nw = otherMeta.w;
                if (otherMeta.w != Invalid)
                    m_metas[otherMeta.w].se = index;
                nw = true;
            }

            if (!ne)
            {
                meta.ne = otherMeta.e;
                if (otherMeta.e != Invalid)
                    m_metas[otherMeta.e].sw = index;
                ne = true;
            }

            if (!w)
            {
                meta.w = otherMeta.sw;
                if (otherMeta.sw != Invalid)
                    m_metas[otherMeta.sw].e = index;
                w = true;
            }

            if (!e)
            {
                meta.e = otherMeta.se;
                if (otherMeta.se != Invalid)
                    m_metas[otherMeta.se].w = index;
                e = true;
            }
        }

        if (!s && (entry = m_map.find(pos + sf::Vector2i(0, 1))) != m_map.end())
        {
            Meta &otherMeta = m_metas[entry->second];

            meta.s = entry->second;
            otherMeta.n = index;

            if (!sw)
            {
                meta.sw = otherMeta.w;
                if (otherMeta.w != Invalid)
                    m_metas[otherMeta.w].ne = index;
                sw = true;
            }

            if (!se)
            {
                meta.se = otherMeta.e;
                if (otherMeta.e != Invalid)
                    m_metas[otherMeta.e].nw = index;
                se = true;
            }

            if (!w)
            {
                meta.w = otherMeta.nw;
                if (otherMeta.nw != Invalid)
                    m_metas[otherMeta.nw].e = index;
                w = true;
            }

            if (!e)
            {
                meta.e = otherMeta.ne;
                if (otherMeta.ne != Invalid)
                    m_metas[otherMeta.ne].w = index;
                e = true;
            }
        }

        if (!w && (entry = m_map.find(pos + sf::Vector2i(-1, 0))) != m_map.end())
        {
            Meta &otherMeta = m_metas[entry->second];

            meta.w = entry->second;
            otherMeta.e = index;

            if (!nw)
            {
                meta.nw = otherMeta.n;
                if (otherMeta.n != Invalid)
                    m_metas[otherMeta.n].se = index;
                nw = true;
            }

            if (!sw)
            {
                meta.sw = otherMeta.s;
                if (otherMeta.s != Invalid)
                    m_metas[otherMeta.s].ne = index;
                sw = true;
            }

            if (!n)
            {
                meta.n = otherMeta.ne;
                if (otherMeta.ne != Invalid)
                    m_metas[otherMeta.ne].s = index;
                n = true;
            }

            if (!s)
            {
                meta.s = otherMeta.se;
                if (otherMeta.se != Invalid)
                    m_metas[otherMeta.se].n = index;
                s = true;
            }
        }

        if (!e && (entry = m_map.find(pos + sf::Vector2i(1, 0))) != m_map.end())
        {
            Meta &otherMeta = m_metas[entry->second];

            meta.e = entry->second;
            otherMeta.w = index;

            if (!ne)
            {
                meta.ne = otherMeta.n;
                if (otherMeta.n != Invalid)
                    m_metas[otherMeta.n].sw = index;
                ne = true;
            }

            if (!se)
            {
                meta.se = otherMeta.s;
                if (otherMeta.s != Invalid)
                    m_metas[otherMeta.s].nw = index;
                se = true;
            }

            if (!n)
            {
                meta.n = otherMeta.nw;
                if (otherMeta.nw != Invalid)
                    m_metas[otherMeta.nw].s = index;
                n = true;
            }

            if (!s)
            {
                meta.s = otherMeta.sw;
                if (otherMeta.sw != Invalid)
                    m_metas[otherMeta.sw].n = index;
                s = true;
            }
        }

        if (!nw && (entry = m_map.find(pos + sf::Vector2i(-1, -1))) != m_map.end())
        {
            Meta &otherMeta = m_metas[entry->second];

            meta.nw = entry->second;
            otherMeta.se = index;

            if (!n)
            {
                meta.n = otherMeta.e;
                if (otherMeta.e != Invalid)
                    m_metas[otherMeta.e].s = index;
                n = true;
            }

            if (!w)
            {
                meta.w = otherMeta.s;
                if (otherMeta.s != Invalid)
                    m_metas[otherMeta.s].e = index;
                w = true;
            }
        }

        if (!ne && (entry = m_map.find(pos + sf::Vector2i(1, -1))) != m_map.end())
        {
            Meta &otherMeta = m_metas[entry->second];

            meta.ne = entry->second;
            otherMeta.sw = index;

            if (!n)
            {
                meta.n = otherMeta.w;
                if (otherMeta.w != Invalid)
                    m_metas[otherMeta.w].s = index;
                n = true;
            }

            if (!e)
            {
                meta.e = otherMeta.s;
                if (otherMeta.s != Invalid)
                    m_metas[otherMeta.s].w = index;
                e = true;
            }
        }

        if (!sw && (entry = m_map.find(pos + sf::Vector2i(-1, 1))) != m_map.end())
        {
            Meta &otherMeta = m_metas[entry->second];

            meta.sw = entry->second;
            otherMeta.ne = index;

            if (!s)
            {
                meta.s = otherMeta.e;
                if (otherMeta.e != Invalid)
                    m_metas[otherMeta.e].n = index;
                s = true;
            }

            if (!w)
            {
                meta.w = otherMeta.n;
                if (otherMeta.n != Invalid)
                    m_metas[otherMeta.n].e = index;
                w = true;
            }
        }

        if (!se && (entry = m_map.find(pos + sf::Vector2i(1, 1))) != m_map.end())
        {
            Meta &otherMeta = m_metas[entry->second];

            meta.se = entry->second;
            otherMeta.nw = index;

            if (!s)
            {
                meta.s = otherMeta.w;
                if (otherMeta.w != Invalid)
                    m_metas[otherMeta.w].n = index;
                s = true;
            }

            if (!e)
            {
                meta.e = otherMeta.n;
                if (otherMeta.n != Invalid)
                    m_metas[otherMeta.n].w = index;
                e = true;
            }
        }

        m_map[pos] = index;
    }

    void disconnect(size_t index)
    {
        Meta &meta = m_metas[index];

        if (meta.n != Invalid)
            m_metas[meta.n].s = Invalid;

        if (meta.s != Invalid)
            m_metas[meta.s].n = Invalid;

        if (meta.w != Invalid)
            m_metas[meta.w].e = Invalid;

        if (meta.e != Invalid)
            m_metas[meta.e].w = Invalid;

        if (meta.nw != Invalid)
            m_metas[meta.nw].se = Invalid;

        if (meta.ne != Invalid)
            m_metas[meta.ne].sw = Invalid;

        if (meta.sw != Invalid)
            m_metas[meta.sw].ne = Invalid;

        if (meta.se != Invalid)
            m_metas[meta.se].nw = Invalid;

        m_map.erase(meta.pos);
    }

public:
    BitBoard() : m_nodes(), m_metas(), m_map(), m_generation(0), m_firstReusable(0) {}
    BitBoard(unsigned int generation) : m_nodes(), m_metas(), m_map(), m_generation(generation), m_firstReusable(0) {}

    BitBoard &set(sf::Vector2i pos, bool state);
    bool get(sf::Vector2i pos) const;

    iterator begin()
    {
        return iterator(&m_nodes, &m_metas, m_generation, 0);
    }

    const_iterator begin() const
    {
        return const_iterator(&m_nodes, &m_metas, m_generation, 0);
    }

    iterator end()
    {
        return iterator(&m_nodes, &m_metas, m_generation, m_nodes.size());
    }

    const_iterator end() const
    {
        return const_iterator(&m_nodes, &m_metas, m_generation, m_nodes.size());
    }

    iterator find(sf::Vector2i pos)
    {
        if (auto entry = m_map.find(pos); entry != m_map.end())
        {
            if (m_nodes[entry->second].generation != m_generation)
                return end();
            return iterator(&m_nodes, &m_metas, m_generation, entry->second);
        }

        return end();
    }

    const_iterator find(sf::Vector2i pos) const
    {
        if (auto entry = m_map.find(pos); entry != m_map.end())
        {
            if (m_nodes[entry->second].generation != m_generation)
                return end();
            return const_iterator(&m_nodes, &m_metas, m_generation, entry->second);
        }

        return end();
    }

    void set(sf::Vector2i pos, Chunk chunk)
    {
        if (auto entry = m_map.find(pos); entry != m_map.end())
        {
            Node &node = m_nodes[entry->second];

            node.chunk = chunk;
            node.generation = m_generation;
        }
        else if (chunk)
        {
            allocate(chunk, pos);
        }
    }

    unsigned int getGeneration() const
    {
        return m_generation;
    }

    void setGeneration(unsigned int generation)
    {
        m_generation = generation;
        m_firstReusable = 0;
    }

    void clear()
    {
        m_nodes.clear();
        m_metas.clear();
        m_map.clear();
    }

    const Chunk &operator[](size_t index) const
    {
        return m_nodes[index].chunk;
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

namespace std
{
    template <typename Node, typename Meta>
    struct tuple_size<typename BitBoard::Proxy<Node, Meta>> : std::integral_constant<size_t, 2>
    {
    };

    template <typename Node, typename Meta>
    struct tuple_element<0, typename BitBoard::Proxy<Node, Meta>>
    {
        using type = Node;
    };

    template <typename Node, typename Meta>
    struct tuple_element<1, typename BitBoard::Proxy<Node, Meta>>
    {
        using type = Meta;
    };
}

template <typename Node, typename Meta, std::size_t I>
decltype(auto) get(typename BitBoard::Proxy<Node, Meta> &e)
{
    if constexpr (I == 0)
        return (e.node);
    else
        return (e.meta);
}

void tick(const BitBoard &previous, BitBoard &buffer);

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
