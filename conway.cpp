#include "conway.hpp"

#include <cmath>
#include <iostream>

BitBoard BitBoard::set(sf::Vector2i pos, bool state) const
{
    BitBoard board = *this;

    sf::Vector2i chunkPos((int)std::floor((float)pos.x / 8.0F), (int)std::floor((float)pos.y / 8.0F));
    sf::Vector2i localPos = pos - (chunkPos * 8);

    int i = (localPos.y * 8) + localPos.x;
    uint64_t mask = 1ULL << i;

    auto entry = board.chunks.find(chunkPos);

    if (entry != board.chunks.end())
    {
        if (uint64_t x = state ? entry->second | mask : entry->second & ~mask)
            entry->second = x;
        else
            board.chunks.erase(entry);
    }
    else if (state)
    {
        board.chunks[chunkPos] = mask;
    }

    return board;
}

bool BitBoard::get(sf::Vector2i pos) const
{
    sf::Vector2i chunkPos((int)std::floor((float)pos.x / 8.0F), (int)std::floor((float)pos.y / 8.0F));
    sf::Vector2i localPos = pos - (chunkPos * 8);

    int i = (localPos.y * 8) + localPos.x;

    auto entry = chunks.find(chunkPos);

    if (entry != chunks.end())
        return entry->second >> i;

    return false;
}

BitBoard &BitBoard::operator|=(const BitBoard &other)
{
    for (auto it = other.chunks.begin(); it != other.chunks.end(); it++)
    {
        auto entry = chunks.find(it->first);

        if (entry != chunks.end())
            entry->second |= it->second;
        else
            chunks[it->first] = it->second;
    }

    return *this;
}

BitBoard &BitBoard::operator-=(const BitBoard &other)
{
    for (auto it = other.chunks.begin(); it != other.chunks.end(); it++)
    {
        auto entry = chunks.find(it->first);

        if (entry != chunks.end())
            if (!(entry->second &= ~it->second))
                chunks.erase(entry);
    }

    return *this;
}

inline static std::pair<uint64_t, uint64_t> halfAdder(uint64_t a, uint64_t b)
{
    return {a ^ b, a & b};
}

inline static std::pair<uint64_t, uint64_t> fullAdder(uint64_t a, uint64_t b, uint64_t c)
{
    uint64_t s = a ^ b;
    return {s ^ c, (a & b) | (s & c)};
}

inline static std::tuple<uint64_t, uint64_t, uint64_t> adder2(uint64_t a0, uint64_t a1, uint64_t b0, uint64_t b1)
{
    auto [s0, c0] = halfAdder(a0, b0);
    auto [s1, c1] = fullAdder(a1, b1, c0);
    return {s0, s1, c1};
}

inline static std::tuple<uint64_t, uint64_t, uint64_t, uint64_t> adder3(uint64_t a0, uint64_t a1, uint64_t a2, uint64_t b0, uint64_t b1, uint64_t b2)
{
    auto [s0, c0] = halfAdder(a0, b0);
    auto [s1, c1] = fullAdder(a1, b1, c0);
    auto [s2, c2] = fullAdder(a2, b2, c1);
    return {s0, s1, s2, c2};
}

inline static uint64_t process(const BitBoard &board, sf::Vector2i pos, uint64_t x, std::unordered_set<sf::Vector2i> *potentialChunks = nullptr)
{
    uint64_t x0 = (x << 1) & 0xFEFEFEFEFEFEFEFEULL; // left neighbor
    uint64_t x1 = (x >> 1) & 0x7F7F7F7F7F7F7F7FULL; // right neighbor
    uint64_t x2 = x << 8;                           // upper neighbor
    uint64_t x3 = x >> 8;                           // lower neighbor
    uint64_t x4 = (x << 9) & 0xFEFEFEFEFEFEFEFEULL; // upper left neighbor
    uint64_t x5 = (x >> 7) & 0xFEFEFEFEFEFEFEFEULL; // lower left neighbor
    uint64_t x6 = (x << 7) & 0x7F7F7F7F7F7F7F7FULL; // upper right neighbor
    uint64_t x7 = (x >> 9) & 0x7F7F7F7F7F7F7F7FULL; // lower right neighbor

    // left border
    sf::Vector2i borderPos = pos + sf::Vector2i(-1, 0);
    auto entry = board.chunks.find(borderPos);

    if (entry != board.chunks.end())
    {
        uint64_t y = entry->second;
        x0 |= (y >> 7) & 0x0101010101010101ULL;
        x4 |= (y << 1) & 0x0101010101010101ULL;
        x5 |= (y >> 15) & 0x0101010101010101ULL;
    }
    else if (potentialChunks && x & 0x0101010101010101ULL)
    {
        potentialChunks->insert(borderPos);
    }

    // right border
    borderPos = pos + sf::Vector2i(1, 0);
    entry = board.chunks.find(borderPos);

    if (entry != board.chunks.end())
    {
        uint64_t y = entry->second;
        x1 |= (y << 7) & 0x8080808080808080ULL;
        x6 |= (y << 15) & 0x8080808080808080ULL;
        x7 |= (y >> 1) & 0x8080808080808080ULL;
    }
    else if (potentialChunks && x & 0x8080808080808080ULL)
    {
        potentialChunks->insert(borderPos);
    }

    // upper border
    borderPos = pos + sf::Vector2i(0, -1);
    entry = board.chunks.find(borderPos);

    if (entry != board.chunks.end())
    {
        uint64_t y = entry->second;
        x2 |= y >> 56;
        x6 |= y >> 57;
        x4 |= (y >> 55) & 0x00000000000000FEULL;
    }
    else if (potentialChunks && x & 0x00000000000000FFULL)
    {
        potentialChunks->insert(borderPos);
    }

    // lower border
    borderPos = pos + sf::Vector2i(0, 1);
    entry = board.chunks.find(borderPos);

    if (entry != board.chunks.end())
    {
        uint64_t y = entry->second;
        x3 |= y << 56;
        x7 |= (y << 55) & 0x7F00000000000000ULL;
        x5 |= y << 57;
    }
    else if (potentialChunks && x & 0xFF00000000000000ULL)
    {
        potentialChunks->insert(borderPos);
    }

    // upper left corner
    borderPos = pos + sf::Vector2i(-1, -1);
    entry = board.chunks.find(borderPos);

    if (entry != board.chunks.end())
    {
        uint64_t y = entry->second;
        x4 |= y >> 63;
    }

    // upper right corner
    borderPos = pos + sf::Vector2i(1, -1);
    entry = board.chunks.find(borderPos);

    if (entry != board.chunks.end())
    {
        uint64_t y = entry->second;
        x6 |= (y >> 49) & 0x0000000000000080ULL;
    }

    // lower left corner
    borderPos = pos + sf::Vector2i(-1, 1);
    entry = board.chunks.find(borderPos);

    if (entry != board.chunks.end())
    {
        uint64_t y = entry->second;
        x5 |= (y << 49) & 0x0100000000000000ULL;
    }

    // lower right corner
    borderPos = pos + sf::Vector2i(1, 1);
    entry = board.chunks.find(borderPos);

    if (entry != board.chunks.end())
    {
        uint64_t y = entry->second;
        x7 |= y << 63;
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

LifeBoard LifeBoard::tick() const
{
    LifeBoard result = *this;
    result.ticks++;

    std::unordered_set<sf::Vector2i> potentialChunks;

    for (auto it = board.chunks.begin(); it != board.chunks.end(); it++)
        if (uint64_t y = process(board, it->first, it->second, &potentialChunks))
        {
            if (y != it->second)
                result.board.chunks[it->first] = y;
        }
        else
            result.board.chunks.erase(it->first);

    for (auto &pos : potentialChunks)
        if (uint64_t y = process(board, pos, 0))
            result.board.chunks[pos] = y;

    return result;
}

static sf::Texture _texture;
const sf::Texture &ChunkRenderer::m_texture(_texture);

void ChunkRenderer::initializeSprites(Logger &logger)
{
    assert(_texture.resize(sf::Vector2u(8, 256)));

    auto [width, height] = _texture.getSize();
    std::vector<std::uint8_t> pixels(width * height * 4);

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
}

void ChunkRenderer::draw(sf::RenderTarget &target, sf::RenderStates states) const
{
    states.transform *= getTransform();

    for (int i = 0; i < 8; i++)
    {
        sf::Sprite sprite(_texture, sf::IntRect({0, int((m_data >> (8 * i)) % 256)}, {8, 1}));
        sprite.setColor(m_color);
        target.draw(sprite, states);

        states.transform = states.transform.translate({0, 1});
    }
}

void BitBoardRenderer::draw(sf::RenderTarget &target, sf::RenderStates states) const
{
    states.transform *= getTransform();

    for (auto it = m_data.chunks.begin(); it != m_data.chunks.end(); it++)
    {
        ChunkRenderer chunk(it->second, m_color);
        chunk.setPosition(static_cast<sf::Vector2f>(it->first * 8));
        target.draw(chunk, states);
    }
}

void LifeBoardRenderer::draw(sf::RenderTarget &target, sf::RenderStates states) const
{
    BitBoardRenderer renderer(m_data.board, m_color);
    target.draw(renderer, states);
}
