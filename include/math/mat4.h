#pragma once

#include <cmath>
#include "math/vec3.h"
#include "math/vec4.h"

namespace math
{
    // Convention used in this study engine:
    // - Row-major storage
    // - Row vectors (v * M)
    // - DirectX clip-space depth [0, 1]
    struct Mat4
    {
        float m[16] = {
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f,
        };
    };

    inline Mat4 Identity()
    {
        return Mat4{};
    }

    inline Mat4 Multiply(const Mat4& a, const Mat4& b)
    {
        Mat4 out{};
        for (int row = 0; row < 4; ++row)
        {
            for (int col = 0; col < 4; ++col)
            {
                out.m[row * 4 + col] =
                    (a.m[row * 4 + 0] * b.m[0 * 4 + col]) +
                    (a.m[row * 4 + 1] * b.m[1 * 4 + col]) +
                    (a.m[row * 4 + 2] * b.m[2 * 4 + col]) +
                    (a.m[row * 4 + 3] * b.m[3 * 4 + col]);
            }
        }
        return out;
    }

    inline Vec4 MultiplyVec4(const Vec4& v, const Mat4& m)
    {
        return Vec4{
            (v.x * m.m[0]) + (v.y * m.m[4]) + (v.z * m.m[8]) + (v.w * m.m[12]),
            (v.x * m.m[1]) + (v.y * m.m[5]) + (v.z * m.m[9]) + (v.w * m.m[13]),
            (v.x * m.m[2]) + (v.y * m.m[6]) + (v.z * m.m[10]) + (v.w * m.m[14]),
            (v.x * m.m[3]) + (v.y * m.m[7]) + (v.z * m.m[11]) + (v.w * m.m[15]),
        };
    }

    inline Vec3 MultiplyPoint(const Vec3& v, const Mat4& m)
    {
        const Vec4 out = MultiplyVec4(Vec4{v.x, v.y, v.z, 1.0f}, m);
        if (std::fabs(out.w) <= 1e-6f)
        {
            return Vec3{out.x, out.y, out.z};
        }
        return Vec3{out.x / out.w, out.y / out.w, out.z / out.w};
    }

    inline Mat4 Translation(const Vec3& t)
    {
        Mat4 out = Identity();
        out.m[12] = t.x;
        out.m[13] = t.y;
        out.m[14] = t.z;
        return out;
    }

    inline Mat4 Scale(const Vec3& s)
    {
        Mat4 out{};
        out.m[0] = s.x;
        out.m[5] = s.y;
        out.m[10] = s.z;
        out.m[15] = 1.0f;
        return out;
    }

    inline Mat4 RotationX(float radians)
    {
        const float c = std::cos(radians);
        const float s = std::sin(radians);
        Mat4 out = Identity();
        out.m[5] = c;
        out.m[6] = s;
        out.m[9] = -s;
        out.m[10] = c;
        return out;
    }

    inline Mat4 RotationY(float radians)
    {
        const float c = std::cos(radians);
        const float s = std::sin(radians);
        Mat4 out = Identity();
        out.m[0] = c;
        out.m[2] = -s;
        out.m[8] = s;
        out.m[10] = c;
        return out;
    }

    inline Mat4 RotationZ(float radians)
    {
        const float c = std::cos(radians);
        const float s = std::sin(radians);
        Mat4 out = Identity();
        out.m[0] = c;
        out.m[1] = s;
        out.m[4] = -s;
        out.m[5] = c;
        return out;
    }

    inline Mat4 RotationEulerXYZ(const Vec3& euler)
    {
        return Multiply(Multiply(RotationX(euler.x), RotationY(euler.y)), RotationZ(euler.z));
    }

    inline Mat4 TRS(const Vec3& t, const Vec3& r, const Vec3& s)
    {
        return Multiply(Multiply(Scale(s), RotationEulerXYZ(r)), Translation(t));
    }

    inline Mat4 LookAtLH(const Vec3& eye, const Vec3& target, const Vec3& worldUp)
    {
        const Vec3 zaxis = Normalize(target - eye);
        const Vec3 xaxis = Normalize(Cross(worldUp, zaxis));
        const Vec3 yaxis = Cross(zaxis, xaxis);

        Mat4 out = Identity();
        out.m[0] = xaxis.x;
        out.m[1] = yaxis.x;
        out.m[2] = zaxis.x;
        out.m[4] = xaxis.y;
        out.m[5] = yaxis.y;
        out.m[6] = zaxis.y;
        out.m[8] = xaxis.z;
        out.m[9] = yaxis.z;
        out.m[10] = zaxis.z;
        out.m[12] = -Dot(xaxis, eye);
        out.m[13] = -Dot(yaxis, eye);
        out.m[14] = -Dot(zaxis, eye);
        return out;
    }

    inline Mat4 LookAtRH(const Vec3& eye, const Vec3& target, const Vec3& worldUp)
    {
        const Vec3 zaxis = Normalize(eye - target);
        const Vec3 xaxis = Normalize(Cross(worldUp, zaxis));
        const Vec3 yaxis = Cross(zaxis, xaxis);

        Mat4 out = Identity();
        out.m[0] = xaxis.x;
        out.m[1] = yaxis.x;
        out.m[2] = zaxis.x;
        out.m[4] = xaxis.y;
        out.m[5] = yaxis.y;
        out.m[6] = zaxis.y;
        out.m[8] = xaxis.z;
        out.m[9] = yaxis.z;
        out.m[10] = zaxis.z;
        out.m[12] = -Dot(xaxis, eye);
        out.m[13] = -Dot(yaxis, eye);
        out.m[14] = -Dot(zaxis, eye);
        return out;
    }

    inline Mat4 PerspectiveLH(float fovRadians, float aspect, float nearPlane, float farPlane)
    {
        const float yScale = 1.0f / std::tan(fovRadians * 0.5f);
        const float xScale = yScale / aspect;

        Mat4 out{};
        out.m[0] = xScale;
        out.m[5] = yScale;
        out.m[10] = farPlane / (farPlane - nearPlane);
        out.m[11] = 1.0f;
        out.m[14] = (-nearPlane * farPlane) / (farPlane - nearPlane);
        out.m[15] = 0.0f;
        return out;
    }

    inline Mat4 PerspectiveRH(float fovRadians, float aspect, float nearPlane, float farPlane)
    {
        const float yScale = 1.0f / std::tan(fovRadians * 0.5f);
        const float xScale = yScale / aspect;

        Mat4 out{};
        out.m[0] = xScale;
        out.m[5] = yScale;
        out.m[10] = farPlane / (nearPlane - farPlane);
        out.m[11] = -1.0f;
        out.m[14] = (nearPlane * farPlane) / (nearPlane - farPlane);
        out.m[15] = 0.0f;
        return out;
    }
}
