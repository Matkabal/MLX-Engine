#pragma once

#include <string>
#include <vector>

#include <windows.h>

namespace editor
{
    class AssetPickerWindow
    {
    public:
        static bool ShowModal(
            HINSTANCE instance,
            const std::string& modelsDirectory,
            const std::string& materialsJsonPath,
            std::string& outSelectedPath);
    };
}
