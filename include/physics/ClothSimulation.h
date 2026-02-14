#pragma once

#include <cstdint>
#include <vector>

#include "geometry/MeshTypes.h"

namespace physics
{
    class ClothSimulation
    {
    public:
        bool Initialize(
            uint32_t columns,
            uint32_t rows,
            float width,
            float height,
            float startY);

        void Step(float dtSeconds, uint32_t solverIterations);
        void SetDragPoint(float ndcX, float ndcY, bool active);

        void BuildVertices(std::vector<geometry::ColoredVertex>& outVertices) const;
        const std::vector<uint32_t>& GetIndices() const { return indices_; }
        uint32_t GetVertexCount() const { return static_cast<uint32_t>(posX_.size()); }

    private:
        struct Constraint
        {
            uint32_t a = 0;
            uint32_t b = 0;
            float restLength = 0.0f;
        };

        uint32_t Index(uint32_t x, uint32_t y) const { return (y * columns_) + x; }
        void SatisfyConstraint(const Constraint& c);
        void BuildConstraints();
        void BuildIndices();
        void ApplyDragging();

    private:
        uint32_t columns_ = 0;
        uint32_t rows_ = 0;
        float width_ = 0.0f;
        float height_ = 0.0f;

        std::vector<float> invMass_;
        std::vector<float> posX_;
        std::vector<float> posY_;
        std::vector<float> prevX_;
        std::vector<float> prevY_;
        std::vector<float> colorR_;
        std::vector<float> colorG_;
        std::vector<float> colorB_;

        std::vector<Constraint> constraints_;
        std::vector<uint32_t> indices_;

        bool dragActive_ = false;
        float dragX_ = 0.0f;
        float dragY_ = 0.0f;
    };
}
