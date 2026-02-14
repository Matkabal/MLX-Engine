#include "renderer/Dx11Renderer.h"

#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

#include "core/Logger.h"
#include "stb_image.h"

namespace renderer
{
    namespace
    {
        std::string BuildMeshKey(const std::string& assetPath, int meshIndex, int primitiveIndex)
        {
            return assetPath + "|m" + std::to_string(meshIndex) + "|p" + std::to_string(primitiveIndex);
        }
    } // namespace

    bool Dx11Renderer::Initialize(
        Dx11Context& context,
        assets::AssetManager& assetManager,
        const assets::MaterialLibrary& materials)
    {
        context_ = &context;
        assetManager_ = &assetManager;
        materials_ = &materials;
        renderObjectCache_.clear();

        auto* device = context.GetDevice();
        if (!device)
        {
            LOG_ERROR("Dx11Renderer", "Initialize", "Device is null.");
            return false;
        }

        if (!CreateFrameConstantsBuffer(device))
        {
            return false;
        }
        if (!CreateMaterialConstantsBuffer(device))
        {
            return false;
        }
        if (!CreateLinearSampler(device))
        {
            return false;
        }
        if (!CreateRasterizerState(device))
        {
            return false;
        }
        if (!CreateAxisGizmoResources(device))
        {
            return false;
        }

        viewMatrix_ = math::LookAtLH(math::Vec3{0.0f, 0.0f, -3.0f}, math::Vec3{0.0f, 0.0f, 0.0f}, math::Vec3{0.0f, 1.0f, 0.0f});
        projectionMatrix_ = math::PerspectiveLH(1.04719755f, 16.0f / 9.0f, 0.1f, 100.0f);
        return true;
    }

    void Dx11Renderer::OnResize(uint32_t width, uint32_t height)
    {
        if (height == 0)
        {
            return;
        }
        projectionMatrix_ = math::PerspectiveLH(1.04719755f, static_cast<float>(width) / static_cast<float>(height), 0.1f, 100.0f);
    }

    void Dx11Renderer::SetCameraMatrices(const math::Mat4& view, const math::Mat4& projection)
    {
        viewMatrix_ = view;
        projectionMatrix_ = projection;
    }

    void Dx11Renderer::RenderFrame(Dx11Context& context, const std::vector<scene::RenderEntity>& renderList)
    {
        auto* deviceContext = context.GetDeviceContext();
        if (!deviceContext || !assetManager_)
        {
            return;
        }

        if (rasterizerState_)
        {
            deviceContext->RSSetState(rasterizerState_.Get());
        }

        const math::Mat4 view = viewMatrix_;
        const math::Mat4 projection = projectionMatrix_;

        if (axisShader_ && axisVertexBuffer_ && axisIndexBuffer_ && axisIndexCount_ > 0)
        {
            deviceContext->IASetInputLayout(axisShader_->GetInputLayout());
            deviceContext->VSSetShader(axisShader_->GetVertexShader(), nullptr, 0);
            deviceContext->PSSetShader(axisShader_->GetPixelShader(), nullptr, 0);
            deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

            const math::Mat4 axisMvp = math::Multiply(math::Multiply(math::Identity(), view), projection);
            FrameConstants frame{};
            std::memcpy(frame.mvp, axisMvp.m, sizeof(frame.mvp));
            D3D11_MAPPED_SUBRESOURCE mapped{};
            HRESULT hr = deviceContext->Map(frameConstantsBuffer_.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
            ThrowIfFailed(hr, "Failed to map frame constants.");
            std::memcpy(mapped.pData, &frame, sizeof(frame));
            deviceContext->Unmap(frameConstantsBuffer_.Get(), 0);
            ID3D11Buffer* vcb = frameConstantsBuffer_.Get();
            deviceContext->VSSetConstantBuffers(0, 1, &vcb);

            MaterialConstants material{};
            material.albedo[0] = 1.0f;
            material.albedo[1] = 1.0f;
            material.albedo[2] = 1.0f;
            material.albedo[3] = 1.0f;
            hr = deviceContext->Map(materialConstantsBuffer_.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
            ThrowIfFailed(hr, "Failed to map material constants.");
            std::memcpy(mapped.pData, &material, sizeof(material));
            deviceContext->Unmap(materialConstantsBuffer_.Get(), 0);
            ID3D11Buffer* pcb = materialConstantsBuffer_.Get();
            deviceContext->PSSetConstantBuffers(0, 1, &pcb);

            ID3D11ShaderResourceView* whiteSrv = GetOrCreateTextureSrv("").Get();
            deviceContext->PSSetShaderResources(0, 1, &whiteSrv);
            if (linearSampler_)
            {
                ID3D11SamplerState* sampler = linearSampler_.Get();
                deviceContext->PSSetSamplers(0, 1, &sampler);
            }

            UINT stride = sizeof(RenderVertex);
            UINT offset = 0;
            deviceContext->IASetVertexBuffers(0, 1, axisVertexBuffer_.GetAddressOf(), &stride, &offset);
            deviceContext->IASetIndexBuffer(axisIndexBuffer_.Get(), DXGI_FORMAT_R32_UINT, 0);
            deviceContext->DrawIndexed(axisIndexCount_, 0, 0);
        }

        for (const scene::RenderEntity& item : renderList)
        {
            if (item.assetPath.empty())
            {
                continue;
            }

            const std::string fullPath = (std::filesystem::path("assets/models") / item.assetPath).string();
            std::string loadError;
            const auto asset = assetManager_->LoadGltf(fullPath, &loadError);
            if (!asset)
            {
                LOG_ERROR("Dx11Renderer", "RenderFrame", loadError);
                continue;
            }

            int meshBegin = 0;
            int meshEnd = static_cast<int>(asset->scene.meshes.size());
            if (item.meshIndex >= 0 && item.meshIndex < meshEnd)
            {
                meshBegin = item.meshIndex;
                meshEnd = item.meshIndex + 1;
            }

            for (int meshIdx = meshBegin; meshIdx < meshEnd; ++meshIdx)
            {
                const auto& mesh = asset->scene.meshes[static_cast<size_t>(meshIdx)];
                int primBegin = 0;
                int primEnd = static_cast<int>(mesh.primitives.size());
                if (item.primitiveIndex >= 0 && item.primitiveIndex < primEnd)
                {
                    primBegin = item.primitiveIndex;
                    primEnd = item.primitiveIndex + 1;
                }

                for (int primIdx = primBegin; primIdx < primEnd; ++primIdx)
                {
                    const std::string key = BuildMeshKey(item.assetPath, meshIdx, primIdx);
                    auto cacheIt = renderObjectCache_.find(key);
                    if (cacheIt == renderObjectCache_.end())
                    {
                        RenderObject built{};
                        std::string buildError;
                        if (!BuildRenderObjectFromPrimitive(item.assetPath, meshIdx, primIdx, built, &buildError))
                        {
                            LOG_ERROR("Dx11Renderer", "RenderFrame", buildError);
                            continue;
                        }
                        cacheIt = renderObjectCache_.emplace(key, std::move(built)).first;
                    }

                    RenderObject& obj = cacheIt->second;
                    if (!obj.shader)
                    {
                        continue;
                    }

                    deviceContext->IASetInputLayout(obj.shader->GetInputLayout());
                    deviceContext->VSSetShader(obj.shader->GetVertexShader(), nullptr, 0);
                    deviceContext->PSSetShader(obj.shader->GetPixelShader(), nullptr, 0);
                    deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

                    const math::Mat4 mvp = math::Multiply(math::Multiply(item.world, view), projection);
                    FrameConstants frame{};
                    std::memcpy(frame.mvp, mvp.m, sizeof(frame.mvp));

                    D3D11_MAPPED_SUBRESOURCE mapped{};
                    HRESULT hr = deviceContext->Map(frameConstantsBuffer_.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
                    ThrowIfFailed(hr, "Failed to map frame constants buffer.");
                    std::memcpy(mapped.pData, &frame, sizeof(frame));
                    deviceContext->Unmap(frameConstantsBuffer_.Get(), 0);
                    ID3D11Buffer* vcb = frameConstantsBuffer_.Get();
                    deviceContext->VSSetConstantBuffers(0, 1, &vcb);

                    MaterialConstants material{};
                    std::memcpy(material.albedo, obj.albedo, sizeof(material.albedo));
                    hr = deviceContext->Map(materialConstantsBuffer_.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
                    ThrowIfFailed(hr, "Failed to map material constants buffer.");
                    std::memcpy(mapped.pData, &material, sizeof(material));
                    deviceContext->Unmap(materialConstantsBuffer_.Get(), 0);
                    ID3D11Buffer* pcb = materialConstantsBuffer_.Get();
                    deviceContext->PSSetConstantBuffers(0, 1, &pcb);

                    ID3D11ShaderResourceView* srv = obj.baseColorSrv ? obj.baseColorSrv.Get() : GetOrCreateTextureSrv("").Get();
                    deviceContext->PSSetShaderResources(0, 1, &srv);
                    if (linearSampler_)
                    {
                        ID3D11SamplerState* sampler = linearSampler_.Get();
                        deviceContext->PSSetSamplers(0, 1, &sampler);
                    }

                    UINT stride = sizeof(RenderVertex);
                    UINT offset = 0;
                    ID3D11Buffer* vb = obj.mesh.vb.handle.Get();
                    deviceContext->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
                    deviceContext->IASetIndexBuffer(obj.mesh.ib.handle.Get(), DXGI_FORMAT_R32_UINT, 0);
                    deviceContext->DrawIndexed(obj.mesh.indexCount, 0, 0);
                }
            }
        }
    }

    bool Dx11Renderer::CreateRenderObjectBuffers(ID3D11Device* device, RenderObject& obj)
    {
        D3D11_BUFFER_DESC vbDesc{};
        vbDesc.Usage = D3D11_USAGE_DEFAULT;
        vbDesc.ByteWidth = static_cast<UINT>(obj.dynamicVertices.size() * sizeof(RenderVertex));
        vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

        D3D11_SUBRESOURCE_DATA vbData{};
        vbData.pSysMem = obj.dynamicVertices.data();
        HRESULT hr = device->CreateBuffer(&vbDesc, &vbData, obj.mesh.vb.handle.GetAddressOf());
        ThrowIfFailed(hr, "Failed to create object vertex buffer.");

        D3D11_BUFFER_DESC ibDesc{};
        ibDesc.Usage = D3D11_USAGE_DEFAULT;
        ibDesc.ByteWidth = static_cast<UINT>(obj.indices.size() * sizeof(uint32_t));
        ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

        D3D11_SUBRESOURCE_DATA ibData{};
        ibData.pSysMem = obj.indices.data();
        hr = device->CreateBuffer(&ibDesc, &ibData, obj.mesh.ib.handle.GetAddressOf());
        ThrowIfFailed(hr, "Failed to create object index buffer.");
        return true;
    }

    bool Dx11Renderer::CreateFrameConstantsBuffer(ID3D11Device* device)
    {
        if (frameConstantsBuffer_)
        {
            return true;
        }
        D3D11_BUFFER_DESC cbDesc{};
        cbDesc.Usage = D3D11_USAGE_DYNAMIC;
        cbDesc.ByteWidth = sizeof(FrameConstants);
        cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        HRESULT hr = device->CreateBuffer(&cbDesc, nullptr, frameConstantsBuffer_.GetAddressOf());
        ThrowIfFailed(hr, "Failed to create frame constants buffer.");
        return true;
    }

    bool Dx11Renderer::CreateMaterialConstantsBuffer(ID3D11Device* device)
    {
        if (materialConstantsBuffer_)
        {
            return true;
        }
        D3D11_BUFFER_DESC cbDesc{};
        cbDesc.Usage = D3D11_USAGE_DYNAMIC;
        cbDesc.ByteWidth = sizeof(MaterialConstants);
        cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        HRESULT hr = device->CreateBuffer(&cbDesc, nullptr, materialConstantsBuffer_.GetAddressOf());
        ThrowIfFailed(hr, "Failed to create material constants buffer.");
        return true;
    }

    bool Dx11Renderer::CreateLinearSampler(ID3D11Device* device)
    {
        if (linearSampler_)
        {
            return true;
        }
        D3D11_SAMPLER_DESC desc{};
        desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
        desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
        desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        desc.MaxLOD = D3D11_FLOAT32_MAX;
        HRESULT hr = device->CreateSamplerState(&desc, linearSampler_.GetAddressOf());
        ThrowIfFailed(hr, "Failed to create linear sampler.");
        return true;
    }

    bool Dx11Renderer::CreateRasterizerState(ID3D11Device* device)
    {
        if (rasterizerState_)
        {
            return true;
        }
        D3D11_RASTERIZER_DESC desc{};
        desc.FillMode = D3D11_FILL_SOLID;
        desc.CullMode = D3D11_CULL_NONE;
        desc.FrontCounterClockwise = TRUE;
        desc.DepthClipEnable = TRUE;
        HRESULT hr = device->CreateRasterizerState(&desc, rasterizerState_.GetAddressOf());
        ThrowIfFailed(hr, "Failed to create rasterizer state.");
        return true;
    }

    bool Dx11Renderer::CreateAxisGizmoResources(ID3D11Device* device)
    {
        if (axisVertexBuffer_ && axisIndexBuffer_ && axisShader_)
        {
            return true;
        }

        D3D11_INPUT_ELEMENT_DESC layout[] = {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0},
        };
        std::string shaderError;
        axisShader_ = shaderLibrary_.GetOrLoad(
            device,
            *materials_,
            "default_color",
            layout,
            ARRAYSIZE(layout),
            &shaderError);
        if (!axisShader_)
        {
            LOG_ERROR("Dx11Renderer", "CreateAxisGizmoResources", shaderError);
            return false;
        }

        const RenderVertex axisVertices[] = {
            {{0.0f, 0.0f, 0.0f}, {1.0f, 0.1f, 0.1f}, {0.0f, 0.0f}}, {{1.5f, 0.0f, 0.0f}, {1.0f, 0.1f, 0.1f}, {1.0f, 0.0f}},
            {{0.0f, 0.0f, 0.0f}, {0.1f, 1.0f, 0.1f}, {0.0f, 0.0f}}, {{0.0f, 1.5f, 0.0f}, {0.1f, 1.0f, 0.1f}, {1.0f, 0.0f}},
            {{0.0f, 0.0f, 0.0f}, {0.1f, 0.3f, 1.0f}, {0.0f, 0.0f}}, {{0.0f, 0.0f, 1.5f}, {0.1f, 0.3f, 1.0f}, {1.0f, 0.0f}},
        };
        const uint32_t axisIndices[] = {0, 1, 2, 3, 4, 5};
        axisIndexCount_ = static_cast<uint32_t>(sizeof(axisIndices) / sizeof(axisIndices[0]));

        D3D11_BUFFER_DESC vbDesc{};
        vbDesc.Usage = D3D11_USAGE_IMMUTABLE;
        vbDesc.ByteWidth = static_cast<UINT>(sizeof(axisVertices));
        vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        D3D11_SUBRESOURCE_DATA vbData{};
        vbData.pSysMem = axisVertices;
        HRESULT hr = device->CreateBuffer(&vbDesc, &vbData, axisVertexBuffer_.GetAddressOf());
        ThrowIfFailed(hr, "Failed to create axis vertex buffer.");

        D3D11_BUFFER_DESC ibDesc{};
        ibDesc.Usage = D3D11_USAGE_IMMUTABLE;
        ibDesc.ByteWidth = static_cast<UINT>(sizeof(axisIndices));
        ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        D3D11_SUBRESOURCE_DATA ibData{};
        ibData.pSysMem = axisIndices;
        hr = device->CreateBuffer(&ibDesc, &ibData, axisIndexBuffer_.GetAddressOf());
        ThrowIfFailed(hr, "Failed to create axis index buffer.");
        return true;
    }

    bool Dx11Renderer::BuildRenderObjectFromPrimitive(
        const std::string& assetFileName,
        int meshIndex,
        int primitiveIndex,
        RenderObject& outObject,
        std::string* outError)
    {
        if (!context_ || !assetManager_ || !materials_)
        {
            if (outError)
            {
                *outError = "Renderer dependencies not initialized.";
            }
            return false;
        }

        const std::string fullPath = (std::filesystem::path("assets/models") / assetFileName).string();
        std::string loadError;
        const auto asset = assetManager_->LoadGltf(fullPath, &loadError);
        if (!asset)
        {
            if (outError)
            {
                *outError = loadError;
            }
            return false;
        }

        if (meshIndex < 0 || meshIndex >= static_cast<int>(asset->scene.meshes.size()))
        {
            if (outError)
            {
                *outError = "Invalid mesh index for asset: " + assetFileName;
            }
            return false;
        }
        const auto& mesh = asset->scene.meshes[static_cast<size_t>(meshIndex)];
        if (primitiveIndex < 0 || primitiveIndex >= static_cast<int>(mesh.primitives.size()))
        {
            if (outError)
            {
                *outError = "Invalid primitive index for asset: " + assetFileName;
            }
            return false;
        }
        const auto& primitive = mesh.primitives[static_cast<size_t>(primitiveIndex)];
        if (primitive.vertices.empty() || primitive.indices.empty())
        {
            if (outError)
            {
                *outError = "Primitive has no geometry data.";
            }
            return false;
        }

        outObject = RenderObject{};
        outObject.dynamicVertices.reserve(primitive.vertices.size());
        outObject.baseVertices.reserve(primitive.vertices.size());
        for (const auto& v : primitive.vertices)
        {
            const bool hasNormal = (v.normal.x != 0.0f) || (v.normal.y != 0.0f) || (v.normal.z != 0.0f);
            const float nx = hasNormal ? v.normal.x : 0.2f;
            const float ny = hasNormal ? v.normal.y : 0.7f;
            const float nz = hasNormal ? v.normal.z : 0.9f;
            RenderVertex rv{
                {v.position.x, v.position.y, v.position.z},
                {(nx * 0.5f) + 0.5f, (ny * 0.5f) + 0.5f, (nz * 0.5f) + 0.5f},
                {v.uv.x, v.uv.y},
            };
            outObject.baseVertices.push_back(rv);
            outObject.dynamicVertices.push_back(rv);
        }
        outObject.indices = primitive.indices;
        outObject.mesh.indexCount = static_cast<uint32_t>(outObject.indices.size());

        const assets::AssetRenderConfig renderConfig = materials_->ResolveRenderConfigForAsset(assetFileName);
        D3D11_INPUT_ELEMENT_DESC layout[] = {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0},
        };
        const std::string shaderId = renderConfig.shaderId.empty()
            ? materials_->ResolveShaderIdForAsset(assetFileName)
            : renderConfig.shaderId;
        std::string shaderError;
        outObject.shader = shaderLibrary_.GetOrLoad(
            context_->GetDevice(),
            *materials_,
            shaderId,
            layout,
            ARRAYSIZE(layout),
            &shaderError);
        if (!outObject.shader)
        {
            if (outError)
            {
                *outError = shaderError;
            }
            return false;
        }

        if (primitive.materialIndex >= 0 && primitive.materialIndex < static_cast<int>(asset->scene.materials.size()))
        {
            const auto& mat = asset->scene.materials[static_cast<size_t>(primitive.materialIndex)];
            outObject.albedo[0] = mat.baseColorFactor.x;
            outObject.albedo[1] = mat.baseColorFactor.y;
            outObject.albedo[2] = mat.baseColorFactor.z;
            outObject.albedo[3] = mat.baseColorFactor.w;
            outObject.baseColorSrv = GetOrCreateTextureSrv(mat.baseColorTexturePath);
        }
        else
        {
            outObject.baseColorSrv = GetOrCreateTextureSrv("");
        }

        return CreateRenderObjectBuffers(context_->GetDevice(), outObject);
    }

    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> Dx11Renderer::GetOrCreateTextureSrv(const std::string& texturePath)
    {
        if (!texturePath.empty())
        {
            const auto it = textureSrvCache_.find(texturePath);
            if (it != textureSrvCache_.end())
            {
                return it->second;
            }
        }
        else if (whiteTextureSrv_)
        {
            return whiteTextureSrv_;
        }

        auto* device = context_ ? context_->GetDevice() : nullptr;
        if (!device)
        {
            return nullptr;
        }

        int width = 1;
        int height = 1;
        int channels = 4;
        unsigned char* pixels = nullptr;
        unsigned char white[4] = {255, 255, 255, 255};
        if (!texturePath.empty())
        {
            pixels = stbi_load(texturePath.c_str(), &width, &height, &channels, 4);
        }
        if (!pixels)
        {
            pixels = white;
            width = 1;
            height = 1;
        }

        D3D11_TEXTURE2D_DESC texDesc{};
        texDesc.Width = static_cast<UINT>(width);
        texDesc.Height = static_cast<UINT>(height);
        texDesc.MipLevels = 1;
        texDesc.ArraySize = 1;
        texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        texDesc.SampleDesc.Count = 1;
        texDesc.Usage = D3D11_USAGE_DEFAULT;
        texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        D3D11_SUBRESOURCE_DATA initData{};
        initData.pSysMem = pixels;
        initData.SysMemPitch = static_cast<UINT>(width * 4);

        Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;
        HRESULT hr = device->CreateTexture2D(&texDesc, &initData, texture.GetAddressOf());
        if (pixels != white)
        {
            stbi_image_free(pixels);
        }
        if (FAILED(hr))
        {
            return nullptr;
        }

        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;
        hr = device->CreateShaderResourceView(texture.Get(), nullptr, srv.GetAddressOf());
        if (FAILED(hr))
        {
            return nullptr;
        }

        if (texturePath.empty())
        {
            whiteTextureSrv_ = srv;
        }
        else
        {
            textureSrvCache_[texturePath] = srv;
        }
        return srv;
    }

    bool Dx11Renderer::BuildMeshFromAssets(
        const std::vector<assets::LoadedGltfAsset>& assets,
        std::vector<RenderVertex>& outVertices,
        std::vector<uint32_t>& outIndices)
    {
        outVertices.clear();
        outIndices.clear();
        uint32_t vertexOffset = 0;

        for (const auto& asset : assets)
        {
            for (const auto& mesh : asset.scene.meshes)
            {
                for (const auto& primitive : mesh.primitives)
                {
                    if (primitive.vertices.empty() || primitive.indices.empty())
                    {
                        continue;
                    }
                    for (const auto& v : primitive.vertices)
                    {
                        outVertices.push_back(RenderVertex{
                            {v.position.x, v.position.y, v.position.z},
                            {0.8f, 0.8f, 0.8f},
                            {v.uv.x, v.uv.y},
                        });
                    }
                    for (uint32_t idx : primitive.indices)
                    {
                        outIndices.push_back(vertexOffset + idx);
                    }
                    vertexOffset += static_cast<uint32_t>(primitive.vertices.size());
                }
            }
        }
        return !outVertices.empty() && !outIndices.empty();
    }

    void Dx11Renderer::ThrowIfFailed(HRESULT hr, const char* message)
    {
        if (FAILED(hr))
        {
            MessageBoxA(nullptr, message, "DX11 Renderer Error", MB_ICONERROR | MB_OK);
            ExitProcess(static_cast<UINT>(hr));
        }
    }
}
