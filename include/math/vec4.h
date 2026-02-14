#pragma once

namespace math
{
    struct Vec4
    {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        float w = 0.0f;
    };

    inline Vec4 operator+(const Vec4& a, const Vec4& b)
    {
        return Vec4{a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w};
    }

    inline Vec4 operator-(const Vec4& a, const Vec4& b)
    {
        return Vec4{a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w};
    }

    inline Vec4 operator*(const Vec4& v, float scalar)
    {
        return Vec4{v.x * scalar, v.y * scalar, v.z * scalar, v.w * scalar};
    }
}
