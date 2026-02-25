#include "conway.hpp"

#include <boost/unordered_set.hpp>
#include <iostream>

constexpr static std::pair<Chunk, Chunk> halfAdder(Chunk a, Chunk b)
{
    return {a ^ b, a & b};
}

constexpr static std::pair<Chunk, Chunk> fullAdder(Chunk a, Chunk b, Chunk c)
{
    Chunk s = a ^ b;
    return {s ^ c, (a & b) | (s & c)};
}

constexpr static std::tuple<Chunk, Chunk, Chunk> adder2(Chunk a0, Chunk a1, Chunk b0, Chunk b1)
{
    auto [s0, c0] = halfAdder(a0, b0);
    auto [s1, c1] = fullAdder(a1, b1, c0);
    return {s0, s1, c1};
}

constexpr static std::tuple<Chunk, Chunk, Chunk, Chunk> adder3(Chunk a0, Chunk a1, Chunk a2, Chunk b0, Chunk b1, Chunk b2)
{
    auto [s0, c0] = halfAdder(a0, b0);
    auto [s1, c1] = fullAdder(a1, b1, c0);
    auto [s2, c2] = fullAdder(a2, b2, c1);
    return {s0, s1, s2, c2};
}

inline static Chunk process(const BitBoard &board, BitBoard::const_iterator it, boost::unordered_set<sf::Vector2i> *potentialChunks = nullptr)
{
    Chunk x0 = it->node.chunk.shiftRight(); // left neighbor
    Chunk x1 = it->node.chunk.shiftLeft();  // right neighbor
    Chunk x2 = it->node.chunk.shiftDown();  // upper neighbor
    Chunk x3 = it->node.chunk.shiftUp();    // lower neighbor
    Chunk x4 = x0.shiftDown();              // upper left neighbor
    Chunk x5 = x0.shiftUp();                // lower left neighbor
    Chunk x6 = x1.shiftDown();              // upper right neighbor
    Chunk x7 = x1.shiftUp();                // lower right neighbor

    if (it->meta.n != BitBoard::Invalid)
    {
        Chunk y = board[it->meta.n].shiftUp(7);
        x2 |= y;
        x6 |= y.shiftLeft();
        x4 |= y.shiftRight();
    }
    else if (potentialChunks && it->node.chunk.shiftDown(7))
    {
        potentialChunks->insert(it->meta.pos + sf::Vector2i(0, -1));
    }

    if (it->meta.s != BitBoard::Invalid)
    {
        Chunk y = board[it->meta.s].shiftDown(7);
        x3 |= y;
        x7 |= y.shiftLeft();
        x5 |= y.shiftRight();
    }
    else if (potentialChunks && it->node.chunk.shiftUp(7))
    {
        potentialChunks->insert(it->meta.pos + sf::Vector2i(0, 1));
    }

    if (it->meta.w != BitBoard::Invalid)
    {
        Chunk y = board[it->meta.w].shiftLeft(7);
        x0 |= y;
        x4 |= y.shiftDown();
        x5 |= y.shiftUp();
    }
    else if (potentialChunks && it->node.chunk.shiftRight(7))
    {
        potentialChunks->insert(it->meta.pos + sf::Vector2i(-1, 0));
    }

    if (it->meta.e != BitBoard::Invalid)
    {
        Chunk y = board[it->meta.e].shiftRight(7);
        x1 |= y;
        x6 |= y.shiftDown();
        x7 |= y.shiftUp();
    }
    else if (potentialChunks && it->node.chunk.shiftLeft(7))
    {
        potentialChunks->insert(it->meta.pos + sf::Vector2i(1, 0));
    }

    if (it->meta.nw != BitBoard::Invalid)
    {
        Chunk y = board[it->meta.nw].shiftLeft(7).shiftUp(7);
        x4 |= y;
    }

    if (it->meta.ne != BitBoard::Invalid)
    {
        Chunk y = board[it->meta.ne].shiftRight(7).shiftUp(7);
        x6 |= y;
    }

    if (it->meta.sw != BitBoard::Invalid)
    {
        Chunk y = board[it->meta.sw].shiftLeft(7).shiftDown(7);
        x5 |= y;
    }

    if (it->meta.se != BitBoard::Invalid)
    {
        Chunk y = board[it->meta.se].shiftRight(7).shiftDown(7);
        x7 |= y;
    }

    auto [s01, c01] = halfAdder(x0, x1);
    auto [s23, c23] = halfAdder(x2, x3);
    auto [s45, c45] = halfAdder(x4, x5);
    auto [s67, c67] = halfAdder(x6, x7);
    auto [q00, q01, c0] = adder2(s01, c01, s23, c23);
    auto [q10, q11, c1] = adder2(s45, c45, s67, c67);
    auto [r0, r1, r2, r3] = adder3(q00, q01, c0, q10, q11, c1);

    return r1 & ~r2 & (r0 | it->node.chunk);
}

inline static Chunk process(const BitBoard &board, sf::Vector2i pos)
{
    Chunk x0; // left neighbor
    Chunk x1; // right neighbor
    Chunk x2; // upper neighbor
    Chunk x3; // lower neighbor
    Chunk x4; // upper left neighbor
    Chunk x5; // lower left neighbor
    Chunk x6; // upper right neighbor
    Chunk x7; // lower right neighbor

    if (auto entry = board.find(pos + sf::Vector2i(0, -1)); entry != board.end())
    {
        Chunk y = entry->node.chunk.shiftUp(7);
        x2 |= y;
        x6 |= y.shiftLeft();
        x4 |= y.shiftRight();
    }

    if (auto entry = board.find(pos + sf::Vector2i(0, 1)); entry != board.end())
    {
        Chunk y = entry->node.chunk.shiftDown(7);
        x3 |= y;
        x7 |= y.shiftLeft();
        x5 |= y.shiftRight();
    }

    if (auto entry = board.find(pos + sf::Vector2i(-1, 0)); entry != board.end())
    {
        Chunk y = entry->node.chunk.shiftLeft(7);
        x0 |= y;
        x4 |= y.shiftDown();
        x5 |= y.shiftUp();
    }

    if (auto entry = board.find(pos + sf::Vector2i(1, 0)); entry != board.end())
    {
        Chunk y = entry->node.chunk.shiftRight(7);
        x1 |= y;
        x6 |= y.shiftDown();
        x7 |= y.shiftUp();
    }

    if (auto entry = board.find(pos + sf::Vector2i(-1, -1)); entry != board.end())
    {
        Chunk y = entry->node.chunk.shiftLeft(7).shiftUp(7);
        x4 |= y;
    }

    if (auto entry = board.find(pos + sf::Vector2i(1, -1)); entry != board.end())
    {
        Chunk y = entry->node.chunk.shiftRight(7).shiftUp(7);
        x6 |= y;
    }

    if (auto entry = board.find(pos + sf::Vector2i(-1, 1)); entry != board.end())
    {
        Chunk y = entry->node.chunk.shiftLeft(7).shiftDown(7);
        x5 |= y;
    }

    if (auto entry = board.find(pos + sf::Vector2i(1, 1)); entry != board.end())
    {
        Chunk y = entry->node.chunk.shiftRight(7).shiftDown(7);
        x7 |= y;
    }

    auto [s01, c01] = halfAdder(x0, x1);
    auto [s23, c23] = halfAdder(x2, x3);
    auto [s45, c45] = halfAdder(x4, x5);
    auto [s67, c67] = halfAdder(x6, x7);
    auto [q00, q01, c0] = adder2(s01, c01, s23, c23);
    auto [q10, q11, c1] = adder2(s45, c45, s67, c67);
    auto [r0, r1, r2, r3] = adder3(q00, q01, c0, q10, q11, c1);

    return r0 & r1 & ~r2;
}

void tick(const BitBoard &previous, BitBoard &buffer)
{
    buffer.setGeneration(previous.getGeneration() + 1);

    boost::unordered_set<sf::Vector2i> potentialChunks;

    for (auto it = previous.begin(); it != previous.end(); it++)
        buffer.set(it->meta.pos, process(previous, it, &potentialChunks));

    for (auto &pos : potentialChunks)
        buffer.set(pos, process(previous, pos));
}

static sf::Texture _texture;
const sf::Texture &ChunkRenderer::m_texture(_texture);

void ChunkRenderer::initializeSprites(Logger &logger)
{
    if (!_texture.resize(sf::Vector2u(8, 256)))
        throw std::runtime_error("Failed to resize the texture.");

    auto [width, height] = _texture.getSize();
    logger.debug("Texture resized successfully to {}x{}.", width, height);

    std::vector<std::uint8_t> pixels(width * height * 4);
    logger.debug("Allocated pixel buffer with {} bytes.", pixels.size());

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
        ChunkRenderer chunk(it->node.chunk, m_color);
        chunk.setPosition(static_cast<sf::Vector2f>(it->meta.pos * 8));
        target.draw(chunk, states);
    }
}
