#include "editor/MaterialEditorWindow.h"
#include "core/Logger.h"

#include <commdlg.h>
#include <filesystem>
#include <fstream>
#include <vector>

#include "editor/ModelImport.h"
#include "json.hpp"

namespace editor
{
    namespace
    {
        constexpr int kListId = 2001;
        constexpr int kAssetEditId = 2002;
        constexpr int kShaderEditId = 2003;
        constexpr int kAddUpdateBtnId = 2004;
        constexpr int kRemoveBtnId = 2005;
        constexpr int kImportBtnId = 2006;
        constexpr int kSaveBtnId = 2007;
        constexpr int kCloseBtnId = 2008;

        struct BindingItem
        {
            std::string asset;
            std::string shaderId;
            nlohmann::json objects = nlohmann::json::array({
                {
                    {"position", nlohmann::json::array({0.0, 0.0, 0.0})},
                    {"rotationDeg", nlohmann::json::array({0.0, 0.0, 0.0})},
                    {"scale", nlohmann::json::array({1.0, 1.0, 1.0})},
                    {"motion", {{"enabled", false}, {"amplitude", 0.0}, {"speed", 1.0}}},
                    {"physics", {{"enabled", false}, {"stiffness", 10.0}, {"damping", 4.0}}},
                },
            });
        };

        struct EditorState
        {
            std::string materialsPath;
            std::string modelsDirectory;
            std::vector<BindingItem> bindings;
            std::vector<nlohmann::json> shaders;
            std::string defaultShaderId = "default_color";
            bool done = false;

            HWND list = nullptr;
            HWND assetEdit = nullptr;
            HWND shaderEdit = nullptr;
        };

        std::wstring ToWide(const std::string& text)
        {
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

        std::string ToUtf8(const std::wstring& text)
        {
            if (text.empty())
            {
                return "";
            }
            const int count = WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, nullptr, 0, nullptr, nullptr);
            std::string out(static_cast<size_t>(count), '\0');
            WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, out.data(), count, nullptr, nullptr);
            if (!out.empty() && out.back() == '\0')
            {
                out.pop_back();
            }
            return out;
        }

        bool LoadJson(EditorState& state)
        {
            state.bindings.clear();
            state.shaders.clear();
            state.defaultShaderId = "default_color";

            nlohmann::json j;
            std::ifstream in(state.materialsPath);
            if (in.is_open())
            {
                try
                {
                    in >> j;
                }
                catch (...)
                {
                    j = nlohmann::json::object();
                }
            }

            if (!j.contains("shaders") || !j["shaders"].is_array())
            {
                j["shaders"] = nlohmann::json::array();
                j["shaders"].push_back({
                    {"id", "default_color"},
                    {"vs", "shaders/triangle_vs.hlsl"},
                    {"ps", "shaders/triangle_ps.hlsl"},
                });
            }
            state.shaders = j["shaders"].get<std::vector<nlohmann::json>>();

            if (j.contains("defaultShaderId") && j["defaultShaderId"].is_string())
            {
                state.defaultShaderId = j["defaultShaderId"].get<std::string>();
            }

            if (j.contains("assetBindings") && j["assetBindings"].is_array())
            {
                for (const auto& item : j["assetBindings"])
                {
                    if (!item.contains("asset") || !item.contains("shaderId"))
                    {
                        continue;
                    }
                    state.bindings.push_back(BindingItem{
                        item["asset"].get<std::string>(),
                        item["shaderId"].get<std::string>(),
                        (item.contains("objects") && item["objects"].is_array()) ? item["objects"] : nlohmann::json::array(),
                    });
                }
            }
            return true;
        }

        bool SaveJson(const EditorState& state, std::string* outError)
        {
            nlohmann::json j;
            j["defaultShaderId"] = state.defaultShaderId;
            j["shaders"] = state.shaders;
            j["assetBindings"] = nlohmann::json::array();

            for (const BindingItem& b : state.bindings)
            {
                j["assetBindings"].push_back({
                    {"asset", b.asset},
                    {"shaderId", b.shaderId},
                    {"objects", b.objects.is_array() ? b.objects : nlohmann::json::array()},
                });
            }

            std::error_code ec;
            std::filesystem::create_directories(std::filesystem::path(state.materialsPath).parent_path(), ec);
            std::ofstream out(state.materialsPath);
            if (!out.is_open())
            {
                if (outError)
                {
                    *outError = "Failed to write materials file.";
                }
                return false;
            }
            out << j.dump(2);
            return true;
        }

        void RefreshList(EditorState* state)
        {
            SendMessageW(state->list, LB_RESETCONTENT, 0, 0);
            for (const BindingItem& b : state->bindings)
            {
                const std::string line = b.asset + " -> " + b.shaderId;
                const std::wstring w = ToWide(line);
                SendMessageW(state->list, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(w.c_str()));
            }
            if (!state->bindings.empty())
            {
                SendMessageW(state->list, LB_SETCURSEL, 0, 0);
            }
        }

        std::string ReadEditText(HWND edit)
        {
            const int len = GetWindowTextLengthW(edit);
            std::wstring w(static_cast<size_t>(len), L'\0');
            GetWindowTextW(edit, w.data(), len + 1);
            return ToUtf8(w);
        }

        void WriteEditText(HWND edit, const std::string& text)
        {
            const std::wstring w = ToWide(text);
            SetWindowTextW(edit, w.c_str());
        }

        bool OpenModelFileDialog(HWND owner, std::string& outPath)
        {
            wchar_t fileName[MAX_PATH] = {};
            OPENFILENAMEW ofn{};
            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner = owner;
            ofn.lpstrFilter = L"glTF Files\0*.gltf;*.glb\0All Files\0*.*\0";
            ofn.lpstrFile = fileName;
            ofn.nMaxFile = MAX_PATH;
            ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
            if (!GetOpenFileNameW(&ofn))
            {
                return false;
            }
            outPath = ToUtf8(fileName);
            return true;
        }

        LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
        {
            auto* state = reinterpret_cast<EditorState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
            switch (msg)
            {
            case WM_NCCREATE:
            {
                const auto* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
                auto* initState = reinterpret_cast<EditorState*>(cs->lpCreateParams);
                SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(initState));
                return DefWindowProcW(hwnd, msg, wParam, lParam);
            }
            case WM_CREATE:
            {
                state->list = CreateWindowExW(
                    WS_EX_CLIENTEDGE, L"LISTBOX", nullptr,
                    WS_CHILD | WS_VISIBLE | LBS_NOTIFY | WS_VSCROLL | WS_BORDER,
                    12, 12, 430, 180, hwnd,
                    reinterpret_cast<HMENU>(static_cast<INT_PTR>(kListId)), nullptr, nullptr);

                CreateWindowExW(0, L"STATIC", L"Asset:", WS_CHILD | WS_VISIBLE, 12, 205, 48, 20, hwnd, nullptr, nullptr, nullptr);
                state->assetEdit = CreateWindowExW(
                    WS_EX_CLIENTEDGE, L"EDIT", nullptr, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                    64, 202, 378, 24, hwnd,
                    reinterpret_cast<HMENU>(static_cast<INT_PTR>(kAssetEditId)), nullptr, nullptr);

                CreateWindowExW(0, L"STATIC", L"Shader:", WS_CHILD | WS_VISIBLE, 12, 236, 48, 20, hwnd, nullptr, nullptr, nullptr);
                state->shaderEdit = CreateWindowExW(
                    WS_EX_CLIENTEDGE, L"EDIT", L"default_color", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                    64, 233, 378, 24, hwnd,
                    reinterpret_cast<HMENU>(static_cast<INT_PTR>(kShaderEditId)), nullptr, nullptr);

                CreateWindowExW(0, L"BUTTON", L"Add/Update", WS_CHILD | WS_VISIBLE, 12, 270, 90, 28, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kAddUpdateBtnId)), nullptr, nullptr);
                CreateWindowExW(0, L"BUTTON", L"Remove", WS_CHILD | WS_VISIBLE, 106, 270, 70, 28, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kRemoveBtnId)), nullptr, nullptr);
                CreateWindowExW(0, L"BUTTON", L"Import Model", WS_CHILD | WS_VISIBLE, 180, 270, 100, 28, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kImportBtnId)), nullptr, nullptr);
                CreateWindowExW(0, L"BUTTON", L"Save", WS_CHILD | WS_VISIBLE, 284, 270, 70, 28, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kSaveBtnId)), nullptr, nullptr);
                CreateWindowExW(0, L"BUTTON", L"Close", WS_CHILD | WS_VISIBLE, 358, 270, 84, 28, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kCloseBtnId)), nullptr, nullptr);

                RefreshList(state);
                return 0;
            }
            case WM_COMMAND:
            {
                const int id = LOWORD(wParam);
                const int code = HIWORD(wParam);
                if (id == kListId && code == LBN_SELCHANGE)
                {
                    const int sel = static_cast<int>(SendMessageW(state->list, LB_GETCURSEL, 0, 0));
                    if (sel >= 0 && sel < static_cast<int>(state->bindings.size()))
                    {
                        WriteEditText(state->assetEdit, state->bindings[static_cast<size_t>(sel)].asset);
                        WriteEditText(state->shaderEdit, state->bindings[static_cast<size_t>(sel)].shaderId);
                    }
                    return 0;
                }
                if (id == kAddUpdateBtnId && code == BN_CLICKED)
                {
                    BindingItem b{};
                    b.asset = ReadEditText(state->assetEdit);
                    b.shaderId = ReadEditText(state->shaderEdit);
                    if (b.asset.empty())
                    {
                        MessageBoxA(hwnd, "Asset cannot be empty.", "Material Editor", MB_OK | MB_ICONWARNING);
                        return 0;
                    }
                    if (b.shaderId.empty())
                    {
                        b.shaderId = "default_color";
                    }

                    namespace fs = std::filesystem;
                    fs::path assetPath(b.asset);
                    if (assetPath.has_parent_path() || fs::exists(assetPath))
                    {
                        std::string importedName;
                        std::string importError;
                        if (!ImportModelWithDependencies(assetPath.string(), state->modelsDirectory, importedName, &importError))
                        {
                            MessageBoxA(hwnd, importError.c_str(), "Import Model Error", MB_OK | MB_ICONERROR);
                            return 0;
                        }
                        b.asset = importedName;
                        WriteEditText(state->assetEdit, b.asset);
                    }
                    else
                    {
                        const fs::path expected = fs::path(state->modelsDirectory) / b.asset;
                        if (!fs::exists(expected))
                        {
                            MessageBoxA(hwnd, "Asset not found in assets/models. Use Import Model first or provide full source path.", "Material Editor", MB_OK | MB_ICONWARNING);
                            return 0;
                        }
                    }

                    const auto it = std::find_if(state->bindings.begin(), state->bindings.end(), [&](const BindingItem& x) { return x.asset == b.asset; });
                    if (it == state->bindings.end())
                    {
                        state->bindings.push_back(b);
                    }
                    else
                    {
                        b.objects = it->objects;
                        *it = b;
                    }
                    RefreshList(state);
                    return 0;
                }
                if (id == kRemoveBtnId && code == BN_CLICKED)
                {
                    const int sel = static_cast<int>(SendMessageW(state->list, LB_GETCURSEL, 0, 0));
                    if (sel >= 0 && sel < static_cast<int>(state->bindings.size()))
                    {
                        state->bindings.erase(state->bindings.begin() + sel);
                        RefreshList(state);
                    }
                    return 0;
                }
                if (id == kImportBtnId && code == BN_CLICKED)
                {
                    std::string sourcePath;
                    if (!OpenModelFileDialog(hwnd, sourcePath))
                    {
                        return 0;
                    }

                    std::string importedName;
                    std::string error;
                    if (!ImportModelWithDependencies(sourcePath, state->modelsDirectory, importedName, &error))
                    {
                        MessageBoxA(hwnd, error.c_str(), "Import Model Error", MB_OK | MB_ICONERROR);
                        return 0;
                    }

                    WriteEditText(state->assetEdit, importedName);
                    MessageBoxA(hwnd, "Model imported to assets/models.", "Material Editor", MB_OK | MB_ICONINFORMATION);
                    return 0;
                }
                if (id == kSaveBtnId && code == BN_CLICKED)
                {
                    std::string err;
                    if (!SaveJson(*state, &err))
                    {
                        MessageBoxA(hwnd, err.c_str(), "Save Error", MB_OK | MB_ICONERROR);
                    }
                    else
                    {
                        MessageBoxA(hwnd, "materials.json saved.", "Material Editor", MB_OK | MB_ICONINFORMATION);
                    }
                    return 0;
                }
                if (id == kCloseBtnId && code == BN_CLICKED)
                {
                    state->done = true;
                    DestroyWindow(hwnd);
                    return 0;
                }
                break;
            }
            case WM_CLOSE:
                state->done = true;
                DestroyWindow(hwnd);
                return 0;
            }
            return DefWindowProcW(hwnd, msg, wParam, lParam);
        }
    } // namespace

    bool MaterialEditorWindow::ShowModal(
        HINSTANCE instance,
        const std::string& materialsJsonPath,
        const std::string& modelsDirectory)
    {
        LOG_METHOD("MaterialEditorWindow", "ShowModal");
        EditorState state{};
        state.materialsPath = materialsJsonPath;
        state.modelsDirectory = modelsDirectory;
        LoadJson(state);

        const wchar_t* className = L"DX11MaterialEditor";
        WNDCLASSEXW wc{};
        wc.cbSize = sizeof(WNDCLASSEXW);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = WndProc;
        wc.hInstance = instance;
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
        wc.lpszClassName = className;
        RegisterClassExW(&wc);

        HWND hwnd = CreateWindowExW(
            0,
            className,
            L"Material Editor",
            WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            470,
            360,
            nullptr,
            nullptr,
            instance,
            &state);

        if (!hwnd)
        {
            return false;
        }

        ShowWindow(hwnd, SW_SHOW);
        UpdateWindow(hwnd);

        MSG msg{};
        while (!state.done && GetMessageW(&msg, nullptr, 0, 0))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        return true;
    }
}
