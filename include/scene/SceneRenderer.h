#pragma once

namespace renderer
{
    class Dx11Context;
    class Dx11Renderer;
}

namespace scene
{
    class Scene;

    class SceneRenderer
    {
    public:
        void Render(Scene& scene, renderer::Dx11Context& context, renderer::Dx11Renderer& renderer) const;
    };
}
