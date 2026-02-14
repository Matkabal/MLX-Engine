#include "physics/ClothSimulation.h"
#include "core/Logger.h"

#include <algorithm>
#include <cmath>

namespace physics
{
    bool ClothSimulation::Initialize(
        uint32_t columns,
        uint32_t rows,
        float width,
        float height,
        float startY)
    {
        LOG_METHOD("ClothSimulation", "Initialize");
        columns_ = std::max<uint32_t>(columns, 2);
        rows_ = std::max<uint32_t>(rows, 2);
        width_ = width;
        height_ = height;

        const uint32_t count = columns_ * rows_;
        invMass_.assign(count, 1.0f);
        posX_.resize(count);
        posY_.resize(count);
        prevX_.resize(count);
        prevY_.resize(count);
        colorR_.resize(count);
        colorG_.resize(count);
        colorB_.resize(count);

        for (uint32_t y = 0; y < rows_; ++y)
        {
            for (uint32_t x = 0; x < columns_; ++x)
            {
                const uint32_t i = Index(x, y);
                const float u = static_cast<float>(x) / static_cast<float>(columns_ - 1);
                const float v = static_cast<float>(y) / static_cast<float>(rows_ - 1);

                posX_[i] = (u - 0.5f) * width_;
                posY_[i] = startY - (v * height_);
                prevX_[i] = posX_[i];
                prevY_[i] = posY_[i];

                const int checker = (static_cast<int>(u * 10.0f) + static_cast<int>(v * 10.0f)) & 1;
                const float base = checker == 0 ? 0.86f : 0.74f;
                colorR_[i] = base;
                colorG_[i] = base * 0.9f;
                colorB_[i] = base * 0.84f;
            }
        }

        // Pin top corners and one midpoint to keep towel hanging.
        invMass_[Index(0, 0)] = 0.0f;
        invMass_[Index(columns_ - 1, 0)] = 0.0f;
        invMass_[Index(columns_ / 2, 0)] = 0.0f;

        BuildConstraints();
        BuildIndices();
        return true;
    }

    void ClothSimulation::Step(float dtSeconds, uint32_t solverIterations)
    {
        LOG_METHOD("ClothSimulation", "Step");
        const float gravity = -1.6f;
        const float dt2 = dtSeconds * dtSeconds;

        for (uint32_t i = 0; i < posX_.size(); ++i)
        {
            if (invMass_[i] == 0.0f)
            {
                continue;
            }

            const float vx = posX_[i] - prevX_[i];
            const float vy = posY_[i] - prevY_[i];

            prevX_[i] = posX_[i];
            prevY_[i] = posY_[i];

            posX_[i] += vx;
            posY_[i] += vy + (gravity * dt2);
        }

        for (uint32_t it = 0; it < solverIterations; ++it)
        {
            for (const Constraint& c : constraints_)
            {
                SatisfyConstraint(c);
            }
            ApplyDragging();
        }
    }

    void ClothSimulation::SetDragPoint(float ndcX, float ndcY, bool active)
    {
        LOG_METHOD("ClothSimulation", "SetDragPoint");
        dragX_ = ndcX;
        dragY_ = ndcY;
        dragActive_ = active;
    }

    void ClothSimulation::BuildVertices(std::vector<geometry::ColoredVertex>& outVertices) const
    {
        LOG_METHOD("ClothSimulation", "BuildVertices");
        outVertices.resize(posX_.size());
        for (uint32_t i = 0; i < posX_.size(); ++i)
        {
            outVertices[i] = geometry::ColoredVertex{
                {posX_[i], posY_[i], 0.0f},
                {colorR_[i], colorG_[i], colorB_[i]},
            };
        }
    }

    void ClothSimulation::SatisfyConstraint(const Constraint& c)
    {
        LOG_METHOD("ClothSimulation", "SatisfyConstraint");
        const float ax = posX_[c.a];
        const float ay = posY_[c.a];
        const float bx = posX_[c.b];
        const float by = posY_[c.b];

        const float dx = bx - ax;
        const float dy = by - ay;
        const float dist = std::sqrt((dx * dx) + (dy * dy));
        if (dist <= 1e-6f)
        {
            return;
        }

        const float diff = (dist - c.restLength) / dist;
        const float wA = invMass_[c.a];
        const float wB = invMass_[c.b];
        const float wSum = wA + wB;
        if (wSum <= 1e-6f)
        {
            return;
        }

        const float corrX = dx * diff;
        const float corrY = dy * diff;

        if (wA > 0.0f)
        {
            posX_[c.a] += corrX * (wA / wSum);
            posY_[c.a] += corrY * (wA / wSum);
        }
        if (wB > 0.0f)
        {
            posX_[c.b] -= corrX * (wB / wSum);
            posY_[c.b] -= corrY * (wB / wSum);
        }
    }

    void ClothSimulation::BuildConstraints()
    {
        LOG_METHOD("ClothSimulation", "BuildConstraints");
        constraints_.clear();
        constraints_.reserve((columns_ - 1) * rows_ + (rows_ - 1) * columns_ + (rows_ - 1) * (columns_ - 1) * 2);

        const auto addConstraint = [this](uint32_t a, uint32_t b) {
            const float dx = posX_[b] - posX_[a];
            const float dy = posY_[b] - posY_[a];
            constraints_.push_back(Constraint{a, b, std::sqrt((dx * dx) + (dy * dy))});
        };

        for (uint32_t y = 0; y < rows_; ++y)
        {
            for (uint32_t x = 0; x < columns_; ++x)
            {
                if (x + 1 < columns_)
                {
                    addConstraint(Index(x, y), Index(x + 1, y));
                }
                if (y + 1 < rows_)
                {
                    addConstraint(Index(x, y), Index(x, y + 1));
                }
                if (x + 1 < columns_ && y + 1 < rows_)
                {
                    addConstraint(Index(x, y), Index(x + 1, y + 1));
                    addConstraint(Index(x + 1, y), Index(x, y + 1));
                }
            }
        }
    }

    void ClothSimulation::BuildIndices()
    {
        LOG_METHOD("ClothSimulation", "BuildIndices");
        indices_.clear();
        indices_.reserve((columns_ - 1) * (rows_ - 1) * 6);

        for (uint32_t y = 0; y < rows_ - 1; ++y)
        {
            for (uint32_t x = 0; x < columns_ - 1; ++x)
            {
                const uint32_t i0 = Index(x, y);
                const uint32_t i1 = Index(x + 1, y);
                const uint32_t i2 = Index(x, y + 1);
                const uint32_t i3 = Index(x + 1, y + 1);

                indices_.push_back(i0);
                indices_.push_back(i1);
                indices_.push_back(i2);

                indices_.push_back(i1);
                indices_.push_back(i3);
                indices_.push_back(i2);
            }
        }
    }

    void ClothSimulation::ApplyDragging()
    {
        LOG_METHOD("ClothSimulation", "ApplyDragging");
        if (!dragActive_)
        {
            return;
        }

        float bestDist2 = 0.03f * 0.03f;
        int bestIndex = -1;
        for (uint32_t i = 0; i < posX_.size(); ++i)
        {
            if (invMass_[i] == 0.0f)
            {
                continue;
            }
            const float dx = posX_[i] - dragX_;
            const float dy = posY_[i] - dragY_;
            const float d2 = (dx * dx) + (dy * dy);
            if (d2 < bestDist2)
            {
                bestDist2 = d2;
                bestIndex = static_cast<int>(i);
            }
        }

        if (bestIndex >= 0)
        {
            posX_[bestIndex] = dragX_;
            posY_[bestIndex] = dragY_;
            prevX_[bestIndex] = dragX_;
            prevY_[bestIndex] = dragY_;
        }
    }
}
