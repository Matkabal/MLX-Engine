#include "renderer/Dx11ShaderProgram.h"
#include "core/Logger.h"

#include <d3dcommon.h>
#include <d3dcompiler.h>
#include <string>
#include <windows.h>

namespace renderer
{
    namespace
    {
        std::string NarrowFromWide(const wchar_t* text)
        {
            if (!text)
            {
                return "";
            }

            const int count = WideCharToMultiByte(CP_UTF8, 0, text, -1, nullptr, 0, nullptr, nullptr);
            if (count <= 1)
            {
                return "";
            }

            std::string out(static_cast<size_t>(count), '\0');
            WideCharToMultiByte(CP_UTF8, 0, text, -1, out.data(), count, nullptr, nullptr);
            if (!out.empty() && out.back() == '\0')
            {
                out.pop_back();
            }
            return out;
        }

        std::string FileStem(const std::string& path)
        {
            const size_t slash = path.find_last_of("/\\");
            const std::string file = (slash == std::string::npos) ? path : path.substr(slash + 1);
            const size_t dot = file.find_last_of('.');
            return (dot == std::string::npos) ? file : file.substr(0, dot);
        }

        void SetDebugName(ID3D11DeviceChild* obj, const std::string& name)
        {
#if defined(_DEBUG)
            if (obj && !name.empty())
            {
                obj->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(name.size()), name.c_str());
            }
#else
            (void)obj;
            (void)name;
#endif
        }
    } // namespace

    bool Dx11ShaderProgram::LoadFromFiles(
        ID3D11Device* device,
        const wchar_t* vertexShaderPath,
        const wchar_t* pixelShaderPath,
        const char* vsEntryPoint,
        const char* psEntryPoint,
        const D3D11_INPUT_ELEMENT_DESC* inputLayoutDesc,
        UINT inputLayoutCount,
        const char* programName)
    {
        LOG_METHOD("Dx11ShaderProgram", "LoadFromFiles");
        if (!device)
        {
            LOG_ERROR("Dx11ShaderProgram", "LoadFromFiles", "Device is null.");
            return false;
        }

        UINT compileFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined(_DEBUG)
        compileFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

        Microsoft::WRL::ComPtr<ID3DBlob> vsBlob;
        Microsoft::WRL::ComPtr<ID3DBlob> psBlob;
        Microsoft::WRL::ComPtr<ID3DBlob> errors;
        const std::string defaultName = FileStem(NarrowFromWide(vertexShaderPath));
        const std::string shaderName = (programName && programName[0] != '\0') ? programName : defaultName;

        HRESULT hr = D3DCompileFromFile(
            vertexShaderPath,
            nullptr,
            D3D_COMPILE_STANDARD_FILE_INCLUDE,
            vsEntryPoint,
            "vs_5_0",
            compileFlags,
            0,
            vsBlob.GetAddressOf(),
            errors.GetAddressOf());
        if (FAILED(hr))
        {
            const char* msg = errors ? static_cast<const char*>(errors->GetBufferPointer()) : "Unknown VS compile error.";
            LOG_ERROR("Dx11ShaderProgram", "LoadFromFiles", msg);
            MessageBoxA(nullptr, msg, "Vertex Shader Compile Error", MB_ICONERROR | MB_OK);
            ExitProcess(static_cast<UINT>(hr));
        }

        hr = D3DCompileFromFile(
            pixelShaderPath,
            nullptr,
            D3D_COMPILE_STANDARD_FILE_INCLUDE,
            psEntryPoint,
            "ps_5_0",
            compileFlags,
            0,
            psBlob.GetAddressOf(),
            errors.ReleaseAndGetAddressOf());
        if (FAILED(hr))
        {
            const char* msg = errors ? static_cast<const char*>(errors->GetBufferPointer()) : "Unknown PS compile error.";
            LOG_ERROR("Dx11ShaderProgram", "LoadFromFiles", msg);
            MessageBoxA(nullptr, msg, "Pixel Shader Compile Error", MB_ICONERROR | MB_OK);
            ExitProcess(static_cast<UINT>(hr));
        }

        hr = device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, vs_.GetAddressOf());
        ThrowIfFailed(hr, "Failed to create vertex shader.");
        SetDebugName(vs_.Get(), shaderName + ".vs");

        hr = device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, ps_.GetAddressOf());
        ThrowIfFailed(hr, "Failed to create pixel shader.");
        SetDebugName(ps_.Get(), shaderName + ".ps");

        hr = device->CreateInputLayout(
            inputLayoutDesc,
            inputLayoutCount,
            vsBlob->GetBufferPointer(),
            vsBlob->GetBufferSize(),
            inputLayout_.GetAddressOf());
        ThrowIfFailed(hr, "Failed to create input layout.");
        SetDebugName(inputLayout_.Get(), shaderName + ".il");

        return true;
    }

    void Dx11ShaderProgram::ThrowIfFailed(HRESULT hr, const char* message)
    {
        LOG_METHOD("Dx11ShaderProgram", "ThrowIfFailed");
        if (FAILED(hr))
        {
            MessageBoxA(nullptr, message, "DX11 Shader Program Error", MB_ICONERROR | MB_OK);
            ExitProcess(static_cast<UINT>(hr));
        }
    }
}
