#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <d3d11.h>
#include <wrl/client.h>

#include "assets/AssetManager.h"
#include "assets/MaterialLibrary.h"
#include "math/mat4.h"
#include "renderer/Dx11Context.h"
#include "renderer/Mesh.h"
#include "renderer/Dx11ShaderProgram.h"
#include "renderer/ShaderLibrary.h"
#include "scene/Scene.h"

namespace renderer
{
    class Dx11Renderer
    {
    public:
        Dx11Renderer() = default;
        ~Dx11Renderer() = default;

        Dx11Renderer(const Dx11Renderer&) = delete;
        Dx11Renderer& operator=(const Dx11Renderer&) = delete;

        bool Initialize(
            Dx11Context& context,
            assets::AssetManager& assetManager,
            const assets::MaterialLibrary& materials);
        void OnResize(uint32_t width, uint32_t height);
        void SetCameraMatrices(const math::Mat4& view, const math::Mat4& projection);
        void RenderFrame(Dx11Context& context, const std::vector<scene::RenderEntity>& renderList);

    private:
        struct RenderVertex
        {
            float position[3];
            float color[3];
            float uv[2];
        };

        struct RenderObject
        {
            std::vector<RenderVertex> baseVertices;
            std::vector<RenderVertex> dynamicVertices;
            std::vector<uint32_t> indices;
            Mesh mesh{};
            std::shared_ptr<Dx11ShaderProgram> shader;
            Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> baseColorSrv;
            float albedo[4] = {1.0f, 1.0f, 1.0f, 1.0f};
        };

        static void ThrowIfFailed(HRESULT hr, const char* message);
        bool CreateRenderObjectBuffers(ID3D11Device* device, RenderObject& obj);
        bool BuildMeshFromAssets(
            const std::vector<assets::LoadedGltfAsset>& assets,
            std::vector<RenderVertex>& outVertices,
            std::vector<uint32_t>& outIndices);
        bool CreateFrameConstantsBuffer(ID3D11Device* device);
        bool CreateMaterialConstantsBuffer(ID3D11Device* device);
        bool CreateLinearSampler(ID3D11Device* device);
        bool CreateRasterizerState(ID3D11Device* device);
        bool CreateAxisGizmoResources(ID3D11Device* device);
        bool BuildRenderObjectFromPrimitive(
            const std::string& assetFileName,
            int meshIndex,
            int primitiveIndex,
            RenderObject& outObject,
            std::string* outError);
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetOrCreateTextureSrv(const std::string& texturePath);

    private:
        struct FrameConstants
        {
            float mvp[16];
        };

        struct MaterialConstants
        {
            float albedo[4];
        };

        ShaderLibrary shaderLibrary_{};
        Microsoft::WRL::ComPtr<ID3D11Buffer> frameConstantsBuffer_;
        Microsoft::WRL::ComPtr<ID3D11Buffer> materialConstantsBuffer_;
        Microsoft::WRL::ComPtr<ID3D11RasterizerState> rasterizerState_;
        Microsoft::WRL::ComPtr<ID3D11SamplerState> linearSampler_;
        Microsoft::WRL::ComPtr<ID3D11Buffer> axisVertexBuffer_;
        Microsoft::WRL::ComPtr<ID3D11Buffer> axisIndexBuffer_;
        uint32_t axisIndexCount_ = 0;
        std::shared_ptr<Dx11ShaderProgram> axisShader_;
        math::Mat4 viewMatrix_ = math::Identity();
        math::Mat4 projectionMatrix_ = math::Identity();
        Dx11Context* context_ = nullptr;
        assets::AssetManager* assetManager_ = nullptr;
        const assets::MaterialLibrary* materials_ = nullptr;
        std::unordered_map<std::string, RenderObject> renderObjectCache_{};
        std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> textureSrvCache_{};
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> whiteTextureSrv_;
    };
}
