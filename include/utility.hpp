#pragma once

#include <SFML/System/Vector2.hpp>
#include <cassert>
#include <functional>

namespace utility
{
    [[nodiscard]] constexpr int floor(float x)
    {
        int i = static_cast<int>(x);
        return i - (static_cast<float>(i) > x);
    }

    [[nodiscard]] constexpr int floorDiv(int x, int y)
    {
        assert(y > 0);
        return (x / y) - (x < 0 && x % y);
    }

    [[nodiscard]] constexpr sf::Vector2i floor(sf::Vector2f v)
    {
        return {floor(v.x), floor(v.y)};
    }

    [[nodiscard]] constexpr sf::Vector2i floorDiv(sf::Vector2i v, sf::Vector2i u)
    {
        return {floorDiv(v.x, u.x), floorDiv(v.y, u.y)};
    }

    // https://dedu.fr/projects/bresenham/
    inline void gridTraversal(sf::Vector2f p1, sf::Vector2f p2, const std::function<void(sf::Vector2i)> &func)
    {
        int ystep = 1;
        int xstep = 1;
        sf::Vector2i d = floor(p2) - floor(p1);

        if (d.y < 0)
        {
            ystep = -1;
            d.y = -d.y;
        }

        if (d.x < 0)
        {
            xstep = -1;
            d.x = -d.x;
        }

        sf::Vector2i dd = 2 * d;
        sf::Vector2i p = floor(p1);
        func(p);

        if (dd.x >= dd.y)
        {
            int errorprev = d.x;
            int error = d.x;

            for (int i = 0; i < d.x; i++)
            {
                p.x += xstep;
                error += dd.y;

                if (error > dd.x)
                {
                    p.y += ystep;
                    error -= dd.x;

                    if (error + errorprev < dd.x)
                        func({p.x, p.y - ystep});
                    else if (error + errorprev > dd.x)
                        func({p.x - xstep, p.y});
                    else
                    {
                        func({p.x, p.y - ystep});
                        func({p.x - xstep, p.y});
                    }
                }

                func(p);
                errorprev = error;
            }
        }
        else
        {
            int errorprev = d.y;
            int error = d.y;

            for (int i = 0; i < d.y; i++)
            {
                p.y += ystep;
                error += dd.x;

                if (error > dd.y)
                {
                    p.x += xstep;
                    error -= dd.y;

                    if (error + errorprev < dd.y)
                        func({p.x - xstep, p.y});
                    else if (error + errorprev > dd.y)
                        func({p.x, p.y - ystep});
                    else
                    {
                        func({p.x - xstep, p.y});
                        func({p.x, p.y - ystep});
                    }
                }

                func(p);
                errorprev = error;
            }
        }
    }
}
