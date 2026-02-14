#include "scene/SceneRenderer.h"

#include "core/Logger.h"
#include "renderer/Dx11Context.h"
#include "renderer/Dx11Renderer.h"
#include "scene/Scene.h"

namespace scene
{
    void SceneRenderer::Render(Scene& scene, renderer::Dx11Context& context, renderer::Dx11Renderer& renderer) const
    {
        LOG_METHOD("SceneRenderer", "Render");
        scene.Update();
        renderer.RenderFrame(context, scene.BuildRenderList());
    }
}
