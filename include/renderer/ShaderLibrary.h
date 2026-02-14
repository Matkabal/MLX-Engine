#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include <d3d11.h>

#include "assets/MaterialLibrary.h"
#include "renderer/Dx11ShaderProgram.h"

namespace renderer
{
    class ShaderLibrary
    {
    public:
        std::shared_ptr<Dx11ShaderProgram> GetOrLoad(
            ID3D11Device* device,
            const assets::MaterialLibrary& materials,
            const std::string& shaderId,
            const D3D11_INPUT_ELEMENT_DESC* inputLayoutDesc,
            UINT inputLayoutCount,
            std::string* outError = nullptr);

    private:
        std::wstring ToWide(const std::string& text) const;

    private:
        std::unordered_map<std::string, std::shared_ptr<Dx11ShaderProgram>> cache_{};
    };
}
