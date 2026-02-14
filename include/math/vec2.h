#pragma once

namespace math
{
    struct Vec2
    {
        float x = 0.0f;
        float y = 0.0f;
    };

    inline Vec2 operator+(const Vec2& a, const Vec2& b)
    {
        return Vec2{a.x + b.x, a.y + b.y};
    }

    inline Vec2 operator-(const Vec2& a, const Vec2& b)
    {
        return Vec2{a.x - b.x, a.y - b.y};
    }

    inline Vec2 operator*(const Vec2& v, float scalar)
    {
        return Vec2{v.x * scalar, v.y * scalar};
    }

    inline Vec2 operator*(float scalar, const Vec2& v)
    {
        return v * scalar;
    }
}
