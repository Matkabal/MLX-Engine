#pragma once

#include <cmath>

namespace math
{
    struct Vec3
    {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
    };

    inline Vec3 operator+(const Vec3& a, const Vec3& b)
    {
        return Vec3{a.x + b.x, a.y + b.y, a.z + b.z};
    }

    inline Vec3 operator-(const Vec3& a, const Vec3& b)
    {
        return Vec3{a.x - b.x, a.y - b.y, a.z - b.z};
    }

    inline Vec3 operator*(const Vec3& v, float scalar)
    {
        return Vec3{v.x * scalar, v.y * scalar, v.z * scalar};
    }

    inline Vec3 operator*(float scalar, const Vec3& v)
    {
        return v * scalar;
    }

    inline float Dot(const Vec3& a, const Vec3& b)
    {
        return (a.x * b.x) + (a.y * b.y) + (a.z * b.z);
    }

    inline Vec3 Cross(const Vec3& a, const Vec3& b)
    {
        return Vec3{
            (a.y * b.z) - (a.z * b.y),
            (a.z * b.x) - (a.x * b.z),
            (a.x * b.y) - (a.y * b.x),
        };
    }

    inline float Length(const Vec3& v)
    {
        return std::sqrt(Dot(v, v));
    }

    inline Vec3 Normalize(const Vec3& v)
    {
        const float len = Length(v);
        if (len <= 1e-6f)
        {
            return Vec3{0.0f, 0.0f, 0.0f};
        }
        return Vec3{v.x / len, v.y / len, v.z / len};
    }
}
