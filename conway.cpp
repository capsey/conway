#include "conway.hpp"

#include <cmath>
#include <iostream>

Chunk &Chunk::set(sf::Vector2i pos, bool state)
{
    int i = (pos.y * 8) + pos.x;
    assert(i >= 0 && i < 64);
    uint64_t mask = 1ULL << i;
    m_data = state ? m_data | mask : m_data & ~mask;
    return *this;
}

bool Chunk::get(sf::Vector2i pos) const
{
    int i = (pos.y * 8) + pos.x;
    assert(i >= 0 && i < 64);
    return (m_data >> i) & 1;
}

void BitBoard::set(sf::Vector2i pos, bool state)
{
    sf::Vector2i chunkPos((int)std::floor((float)pos.x / 8.0F), (int)std::floor((float)pos.y / 8.0F));
    sf::Vector2i localPos = pos - (chunkPos * 8);

    auto entry = m_chunks.find(chunkPos);

    if (entry != m_chunks.end())
    {
        if (!entry->second.set(localPos, state))
            m_chunks.erase(entry);
    }
    else if (state)
    {
        m_chunks[chunkPos].set(localPos, state);
    }
}

bool BitBoard::get(sf::Vector2i pos) const
{
    sf::Vector2i chunkPos((int)std::floor((float)pos.x / 8.0F), (int)std::floor((float)pos.y / 8.0F));
    sf::Vector2i localPos = pos - (chunkPos * 8);

    auto entry = m_chunks.find(chunkPos);

    if (entry != m_chunks.end())
        return entry->second.get(localPos);

    return false;
}

BitBoard &BitBoard::operator|=(const BitBoard &other)
{
    for (auto it = other.m_chunks.begin(); it != other.m_chunks.end(); it++)
    {
        auto entry = m_chunks.find(it->first);

        if (entry != m_chunks.end())
            entry->second |= it->second;
        else
            m_chunks[it->first] = it->second;
    }

    return *this;
}

BitBoard &BitBoard::operator-=(const BitBoard &other)
{
    for (auto it = other.m_chunks.begin(); it != other.m_chunks.end(); it++)
    {
        auto entry = m_chunks.find(it->first);

        if (entry != m_chunks.end())
            if (!(entry->second &= ~it->second))
                m_chunks.erase(entry);
    }

    return *this;
}

inline static std::pair<Chunk, Chunk> halfAdder(Chunk a, Chunk b)
{
    return {a ^ b, a & b};
}

inline static std::pair<Chunk, Chunk> fullAdder(Chunk a, Chunk b, Chunk c)
{
    Chunk s = a ^ b;
    return {s ^ c, (a & b) | (s & c)};
}

inline static std::tuple<Chunk, Chunk, Chunk> adder2(Chunk a0, Chunk a1, Chunk b0, Chunk b1)
{
    auto [s0, c0] = halfAdder(a0, b0);
    auto [s1, c1] = fullAdder(a1, b1, c0);
    return {s0, s1, c1};
}

inline static std::tuple<Chunk, Chunk, Chunk, Chunk> adder3(Chunk a0, Chunk a1, Chunk a2, Chunk b0, Chunk b1, Chunk b2)
{
    auto [s0, c0] = halfAdder(a0, b0);
    auto [s1, c1] = fullAdder(a1, b1, c0);
    auto [s2, c2] = fullAdder(a2, b2, c1);
    return {s0, s1, s2, c2};
}

inline static Chunk process(const BitBoard &board, sf::Vector2i pos, Chunk x, std::unordered_set<sf::Vector2i> *potentialChunks = nullptr)
{
    Chunk x0 = x.shiftRight(); // left neighbor
    Chunk x1 = x.shiftLeft();  // right neighbor
    Chunk x2 = x.shiftDown();  // upper neighbor
    Chunk x3 = x.shiftUp();    // lower neighbor
    Chunk x4 = x0.shiftDown(); // upper left neighbor
    Chunk x5 = x0.shiftUp();   // lower left neighbor
    Chunk x6 = x1.shiftDown(); // upper right neighbor
    Chunk x7 = x1.shiftUp();   // lower right neighbor

    // left border
    sf::Vector2i borderPos = pos + sf::Vector2i(-1, 0);
    auto entry = board.find(borderPos);

    if (entry != board.end())
    {
        Chunk y = entry->second.shiftLeft(7);
        x0 |= y;
        x4 |= y.shiftDown();
        x5 |= y.shiftUp();
    }
    else if (potentialChunks && x.shiftRight(7))
    {
        potentialChunks->insert(borderPos);
    }

    // right border
    borderPos = pos + sf::Vector2i(1, 0);
    entry = board.find(borderPos);

    if (entry != board.end())
    {
        Chunk y = entry->second.shiftRight(7);
        x1 |= y;
        x6 |= y.shiftDown();
        x7 |= y.shiftUp();
    }
    else if (potentialChunks && x.shiftLeft(7))
    {
        potentialChunks->insert(borderPos);
    }

    // upper border
    borderPos = pos + sf::Vector2i(0, -1);
    entry = board.find(borderPos);

    if (entry != board.end())
    {
        Chunk y = entry->second.shiftUp(7);
        x2 |= y;
        x6 |= y.shiftLeft();
        x4 |= y.shiftRight();
    }
    else if (potentialChunks && x.shiftDown(7))
    {
        potentialChunks->insert(borderPos);
    }

    // lower border
    borderPos = pos + sf::Vector2i(0, 1);
    entry = board.find(borderPos);

    if (entry != board.end())
    {
        Chunk y = entry->second.shiftDown(7);
        x3 |= y;
        x7 |= y.shiftLeft();
        x5 |= y.shiftRight();
    }
    else if (potentialChunks && x.shiftUp(7))
    {
        potentialChunks->insert(borderPos);
    }

    // upper left corner
    borderPos = pos + sf::Vector2i(-1, -1);
    entry = board.find(borderPos);

    if (entry != board.end())
    {
        Chunk y = entry->second.shiftLeft(7).shiftUp(7);
        x4 |= y;
    }

    // upper right corner
    borderPos = pos + sf::Vector2i(1, -1);
    entry = board.find(borderPos);

    if (entry != board.end())
    {
        Chunk y = entry->second.shiftRight(7).shiftUp(7);
        x6 |= y;
    }

    // lower left corner
    borderPos = pos + sf::Vector2i(-1, 1);
    entry = board.find(borderPos);

    if (entry != board.end())
    {
        Chunk y = entry->second.shiftLeft(7).shiftDown(7);
        x5 |= y;
    }

    // lower right corner
    borderPos = pos + sf::Vector2i(1, 1);
    entry = board.find(borderPos);

    if (entry != board.end())
    {
        Chunk y = entry->second.shiftRight(7).shiftDown(7);
        x7 |= y;
    }

    // 1-bit layer
    auto [s01, c01] = halfAdder(x0, x1);
    auto [s23, c23] = halfAdder(x2, x3);
    auto [s45, c45] = halfAdder(x4, x5);
    auto [s67, c67] = halfAdder(x6, x7);

    // 2-bit layer
    auto [q00, q01, c0] = adder2(s01, c01, s23, c23);
    auto [q10, q11, c1] = adder2(s45, c45, s67, c67);

    // 3-bit layer
    auto [r0, r1, r2, r3] = adder3(q00, q01, c0, q10, q11, c1);

    return r1 & ~r2 & (r0 | x);
}

LifeBoard LifeBoard::next() const
{
    LifeBoard result = *this;
    result.m_ticks++;

    std::unordered_set<sf::Vector2i> potentialChunks;

    for (auto it = m_board.begin(); it != m_board.end(); it++)
        if (Chunk y = process(m_board, it->first, it->second, &potentialChunks))
        {
            if (y != it->second)
                result.m_board[it->first] = std::move(y);
        }
        else
            result.m_board.erase(it->first);

    for (auto &pos : potentialChunks)
        if (Chunk y = process(m_board, pos, Chunk()))
            result.m_board[pos] = std::move(y);

    return result;
}

static sf::Texture _texture;
const sf::Texture &ChunkRenderer::m_texture(_texture);

void ChunkRenderer::initializeSprites(Logger &logger)
{
    if (!_texture.resize(sf::Vector2u(8, 256)))
        throw std::runtime_error("Failed to resize the texture.");

    auto [width, height] = _texture.getSize();
    logger.debug("Texture resized successfully to {}x{}", width, height);

    std::vector<std::uint8_t> pixels(width * height * 4);
    logger.debug("Allocated pixel buffer ({} bytes)", pixels.size());

    for (unsigned int i = 0; i < height; i++)
    {
        unsigned int n = i;

        for (unsigned int j = 0; j < width; j++)
        {
            const uint8_t value = (n & 1) ? 255 : 0;
            const size_t index = (i * width + j) * 4;

            pixels[index + 0] = value;
            pixels[index + 1] = value;
            pixels[index + 2] = value;
            pixels[index + 3] = value;

            n >>= 1;
        }
    }

    _texture.update(pixels.data());
    logger.info("Sprite texture initialization completed successfully.");
}

void ChunkRenderer::draw(sf::RenderTarget &target, sf::RenderStates states) const
{
    states.transform *= getTransform();

    for (int i = 0; i < 8; i++)
    {
        sf::Sprite sprite(_texture, sf::IntRect({0, int((m_data.data() >> (8 * i)) % 256)}, {8, 1}));
        sprite.setColor(m_color);
        target.draw(sprite, states);

        states.transform = states.transform.translate({0, 1});
    }
}

void BitBoardRenderer::draw(sf::RenderTarget &target, sf::RenderStates states) const
{
    states.transform *= getTransform();

    for (auto it = m_data.begin(); it != m_data.end(); it++)
    {
        ChunkRenderer chunk(it->second, m_color);
        chunk.setPosition(static_cast<sf::Vector2f>(it->first * 8));
        target.draw(chunk, states);
    }
}

void LifeBoardRenderer::draw(sf::RenderTarget &target, sf::RenderStates states) const
{
    BitBoardRenderer renderer(m_data.board(), m_color);
    target.draw(renderer, states);
}
