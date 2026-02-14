#pragma once

#include <d3d11.h>
#include <wrl/client.h>

namespace renderer
{
    class Dx11ShaderProgram
    {
    public:
        bool LoadFromFiles(
            ID3D11Device* device,
            const wchar_t* vertexShaderPath,
            const wchar_t* pixelShaderPath,
            const char* vsEntryPoint,
            const char* psEntryPoint,
            const D3D11_INPUT_ELEMENT_DESC* inputLayoutDesc,
            UINT inputLayoutCount,
            const char* programName = nullptr);

        ID3D11VertexShader* GetVertexShader() const { return vs_.Get(); }
        ID3D11PixelShader* GetPixelShader() const { return ps_.Get(); }
        ID3D11InputLayout* GetInputLayout() const { return inputLayout_.Get(); }

    private:
        static void ThrowIfFailed(HRESULT hr, const char* message);

    private:
        Microsoft::WRL::ComPtr<ID3D11VertexShader> vs_;
        Microsoft::WRL::ComPtr<ID3D11PixelShader> ps_;
        Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout_;
    };
}
