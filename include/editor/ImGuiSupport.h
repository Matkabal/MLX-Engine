#pragma once

// Build-time feature toggle:
// If Dear ImGui headers are available in include path, editor panels are enabled.
#if __has_include("imgui.h")
#define DX11_WITH_IMGUI 1
#include "imgui.h"
#else
#define DX11_WITH_IMGUI 0
#endif

