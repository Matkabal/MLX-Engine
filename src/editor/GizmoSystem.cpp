#include "editor/GizmoSystem.h"

#include <cmath>

#include "scene/components/TransformComponent.h"

namespace editor
{
    namespace
    {
        float AbsDot(const math::Vec3& a, const math::Vec3& b)
        {
            const float d = math::Dot(a, b);
            return (d >= 0.0f) ? d : -d;
        }
    }

    void GizmoSystem::Update(scene::Scene& scene, ecs::Entity selected, const SceneCamera& camera, const GizmoInput& input)
    {
        if (selected == ecs::kInvalidEntity || mode_ != GizmoMode::Translate)
        {
            activeAxis_ = Axis::None;
            return;
        }

        auto* transform = scene.Components().Get<scene::TransformComponent>(selected);
        if (!transform)
        {
            activeAxis_ = Axis::None;
            return;
        }

        const Ray ray = camera.BuildPickRay(input.ndcX, input.ndcY, input.aspect);
        const math::Vec3 center = transform->world.m[12] == transform->world.m[12]
            ? math::Vec3{transform->world.m[12], transform->world.m[13], transform->world.m[14]}
            : transform->local.position;

        if (input.leftPressed)
        {
            activeAxis_ = PickAxis(center, ray);
            if (activeAxis_ != Axis::None)
            {
                dragOriginWorld_ = center;
                initialPosition_ = transform->local.position;
                const math::Vec3 axisDir = AxisDirection(activeAxis_);
                dragPlaneNormal_ = BuildDragPlaneNormal(axisDir, ray);
                const PlaneHit startHit = IntersectRayPlane(ray, dragOriginWorld_, dragPlaneNormal_);
                if (startHit.hit)
                {
                    dragStartPoint_ = startHit.point;
                }
                else
                {
                    activeAxis_ = Axis::None;
                }
            }
        }

        if (activeAxis_ != Axis::None && input.leftHeld)
        {
            const PlaneHit hit = IntersectRayPlane(ray, dragOriginWorld_, dragPlaneNormal_);
            if (!hit.hit)
            {
                return;
            }

            // Delta projection:
            // move amount is projection of drag vector over selected gizmo axis.
            const math::Vec3 axisDir = AxisDirection(activeAxis_);
            const math::Vec3 drag = hit.point - dragStartPoint_;
            const float move = math::Dot(drag, axisDir);
            transform->local.position = initialPosition_ + (axisDir * move);
        }

        if (input.leftReleased)
        {
            activeAxis_ = Axis::None;
        }
    }

    GizmoSystem::PlaneHit GizmoSystem::IntersectRayPlane(const Ray& ray, const math::Vec3& planePoint, const math::Vec3& planeNormal)
    {
        PlaneHit out{};
        const float denom = math::Dot(planeNormal, ray.direction);
        if (std::fabs(denom) <= 1e-5f)
        {
            return out;
        }

        const float t = math::Dot(planePoint - ray.origin, planeNormal) / denom;
        if (t < 0.0f)
        {
            return out;
        }

        out.hit = true;
        out.point = ray.origin + (ray.direction * t);
        return out;
    }

    GizmoSystem::Axis GizmoSystem::PickAxis(const math::Vec3& center, const Ray& ray) const
    {
        constexpr float axisLength = 1.2f;
        constexpr float pickRadius = 0.12f;

        auto distanceToSegment = [&](const math::Vec3& a, const math::Vec3& b) {
            const math::Vec3 ab = b - a;
            const math::Vec3 ao = ray.origin - a;
            const math::Vec3 n = math::Cross(ray.direction, ab);
            const float denom = math::Length(n);
            if (denom <= 1e-5f)
            {
                return 1e9f;
            }
            return std::fabs(math::Dot(ao, n) / denom);
        };

        const float dx = distanceToSegment(center, center + math::Vec3{axisLength, 0.0f, 0.0f});
        const float dy = distanceToSegment(center, center + math::Vec3{0.0f, axisLength, 0.0f});
        const float dz = distanceToSegment(center, center + math::Vec3{0.0f, 0.0f, axisLength});

        float best = pickRadius;
        Axis bestAxis = Axis::None;
        if (dx < best) { best = dx; bestAxis = Axis::X; }
        if (dy < best) { best = dy; bestAxis = Axis::Y; }
        if (dz < best) { best = dz; bestAxis = Axis::Z; }
        return bestAxis;
    }

    math::Vec3 GizmoSystem::AxisDirection(Axis axis) const
    {
        switch (axis)
        {
        case Axis::X:
            return math::Vec3{1.0f, 0.0f, 0.0f};
        case Axis::Y:
            return math::Vec3{0.0f, 1.0f, 0.0f};
        case Axis::Z:
            return math::Vec3{0.0f, 0.0f, 1.0f};
        default:
            return math::Vec3{1.0f, 0.0f, 0.0f};
        }
    }

    math::Vec3 GizmoSystem::BuildDragPlaneNormal(const math::Vec3& axisDir, const Ray& ray) const
    {
        // Drag plane normal is orthogonal to the gizmo axis and as stable as possible
        // relative to the view direction.
        math::Vec3 n = math::Normalize(math::Cross(axisDir, ray.direction));
        if (math::Length(n) <= 1e-4f)
        {
            n = math::Vec3{0.0f, 1.0f, 0.0f};
        }
        math::Vec3 planeNormal = math::Normalize(math::Cross(axisDir, n));

        if (AbsDot(planeNormal, axisDir) > 0.99f || math::Length(planeNormal) <= 1e-4f)
        {
            planeNormal = math::Vec3{0.0f, 0.0f, 1.0f};
        }
        return planeNormal;
    }
}

