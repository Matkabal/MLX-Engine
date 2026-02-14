#pragma once

#include <string>

#include <windows.h>

namespace editor
{
    class ProjectBrowserWindow
    {
    public:
        static bool ShowModal(HINSTANCE instance, const std::string& projectsRoot, std::string& outProjectPath);
    };
}
