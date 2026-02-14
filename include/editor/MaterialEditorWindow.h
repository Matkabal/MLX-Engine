#pragma once

#include <string>

#include <windows.h>

namespace editor
{
    class MaterialEditorWindow
    {
    public:
        static bool ShowModal(
            HINSTANCE instance,
            const std::string& materialsJsonPath,
            const std::string& modelsDirectory);
    };
}
