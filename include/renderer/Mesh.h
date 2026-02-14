#pragma once

#include <cstdint>

#include <d3d11.h>
#include <wrl/client.h>

namespace renderer
{
    struct VertexBuffer
    {
        Microsoft::WRL::ComPtr<ID3D11Buffer> handle;
    };

    struct IndexBuffer
    {
        Microsoft::WRL::ComPtr<ID3D11Buffer> handle;
    };

    struct Mesh
    {
        VertexBuffer vb;
        IndexBuffer ib;
        uint32_t indexCount = 0;
    };
}
