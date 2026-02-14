#pragma once

#include "ecs/Entity.h"
#include "editor/SceneCamera.h"
#include "scene/Scene.h"

namespace editor
{
    enum class GizmoMode
    {
        Translate,
        Rotate,
        Scale,
    };

    struct GizmoInput
    {
        bool leftPressed = false;
        bool leftHeld = false;
        bool leftReleased = false;
        float ndcX = 0.0f;
        float ndcY = 0.0f;
        float aspect = 1.0f;
    };

    class GizmoSystem
    {
    public:
        void SetMode(GizmoMode mode) { mode_ = mode; }
        GizmoMode GetMode() const { return mode_; }

        void Update(scene::Scene& scene, ecs::Entity selected, const SceneCamera& camera, const GizmoInput& input);

    private:
        enum class Axis
        {
            None,
            X,
            Y,
            Z,
        };

        struct PlaneHit
        {
            bool hit = false;
            math::Vec3 point{};
        };

        static PlaneHit IntersectRayPlane(const Ray& ray, const math::Vec3& planePoint, const math::Vec3& planeNormal);
        Axis PickAxis(const math::Vec3& center, const Ray& ray) const;
        math::Vec3 AxisDirection(Axis axis) const;
        math::Vec3 BuildDragPlaneNormal(const math::Vec3& axisDir, const Ray& ray) const;

    private:
        GizmoMode mode_ = GizmoMode::Translate;
        Axis activeAxis_ = Axis::None;
        math::Vec3 dragOriginWorld_{};
        math::Vec3 dragStartPoint_{};
        math::Vec3 dragPlaneNormal_{};
        math::Vec3 initialPosition_{};
    };
}

