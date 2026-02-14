#include "renderer/ShaderLibrary.h"
#include "core/Logger.h"

#include <windows.h>

namespace renderer
{
    std::shared_ptr<Dx11ShaderProgram> ShaderLibrary::GetOrLoad(
        ID3D11Device* device,
        const assets::MaterialLibrary& materials,
        const std::string& shaderId,
        const D3D11_INPUT_ELEMENT_DESC* inputLayoutDesc,
        UINT inputLayoutCount,
        std::string* outError)
    {
        LOG_METHOD("ShaderLibrary", "GetOrLoad");
        if (!device)
        {
            LOG_ERROR("ShaderLibrary", "GetOrLoad", "Invalid D3D11 device.");
            if (outError)
            {
                *outError = "Invalid D3D11 device for shader loading.";
            }
            return nullptr;
        }

        const auto it = cache_.find(shaderId);
        if (it != cache_.end())
        {
            return it->second;
        }

        assets::ShaderDefinition def{};
        if (!materials.TryGetShaderDefinition(shaderId, def))
        {
            LOG_ERROR("ShaderLibrary", "GetOrLoad", std::string("Unknown shader id: ") + shaderId);
            if (outError)
            {
                *outError = "Shader id not found in MaterialLibrary: " + shaderId;
            }
            return nullptr;
        }

        auto program = std::make_shared<Dx11ShaderProgram>();
        if (!program->LoadFromFiles(
                device,
                ToWide(def.vertexShaderPath).c_str(),
                ToWide(def.pixelShaderPath).c_str(),
                "VSMain",
                "PSMain",
                inputLayoutDesc,
                inputLayoutCount,
                shaderId.c_str()))
        {
            if (outError)
            {
                *outError = "Failed to compile/load shader program: " + shaderId;
            }
            return nullptr;
        }

        cache_[shaderId] = program;
        return program;
    }

    std::wstring ShaderLibrary::ToWide(const std::string& text) const
    {
        LOG_METHOD("ShaderLibrary", "ToWide");
        if (text.empty())
        {
            return L"";
        }

        const int count = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
        std::wstring out(static_cast<size_t>(count), L'\0');
        MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, out.data(), count);
        if (!out.empty() && out.back() == L'\0')
        {
            out.pop_back();
        }
        return out;
    }
}
