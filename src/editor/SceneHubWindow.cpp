#include "editor/SceneHubWindow.h"

#include <filesystem>
#include <fstream>
#include <vector>

#include "assets/AssetRegistry.h"
#include "assets/SceneRepository.h"
#include "core/Logger.h"

namespace editor
{
    namespace
    {
        constexpr int kScenesListId = 4001;
        constexpr int kAssetsListId = 4002;
        constexpr int kSceneNameEditId = 4003;
        constexpr int kNewSceneBtnId = 4004;
        constexpr int kDeleteSceneBtnId = 4005;
        constexpr int kOpenBtnId = 4006;
        constexpr int kCancelBtnId = 4007;

        struct State
        {
            std::string projectPath;
            std::string modelsDir;
            std::vector<std::string> scenePaths;
            std::vector<std::string> assetPaths;
            std::string selectedScene;
            std::string selectedAsset;
            bool done = false;
            bool accepted = false;
            HWND scenesList = nullptr;
            HWND assetsList = nullptr;
            HWND sceneEdit = nullptr;
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

        std::string ReadEdit(HWND edit)
        {
            const int len = GetWindowTextLengthW(edit);
            std::wstring w(static_cast<size_t>(len), L'\0');
            GetWindowTextW(edit, w.data(), len + 1);
            return ToUtf8(w);
        }

        void RefreshScenes(State* s)
        {
            namespace fs = std::filesystem;
            s->scenePaths.clear();
            const fs::path sceneDir = fs::path(s->projectPath) / "scenes";
            std::error_code ec;
            fs::create_directories(sceneDir, ec);
            for (const auto& entry : fs::directory_iterator(sceneDir, ec))
            {
                if (entry.is_regular_file() && entry.path().extension() == ".json")
                {
                    s->scenePaths.push_back(entry.path().string());
                }
            }

            SendMessageW(s->scenesList, LB_RESETCONTENT, 0, 0);
            for (const auto& scenePath : s->scenePaths)
            {
                const std::wstring name = ToWide(std::filesystem::path(scenePath).filename().string());
                SendMessageW(s->scenesList, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(name.c_str()));
            }
            if (!s->scenePaths.empty())
            {
                SendMessageW(s->scenesList, LB_SETCURSEL, 0, 0);
            }
        }

        void RefreshAssets(State* s)
        {
            assets::AssetRegistry reg;
            std::string err;
            reg.ScanGltf(s->modelsDir, &err);
            s->assetPaths = reg.GetAssetPaths();

            SendMessageW(s->assetsList, LB_RESETCONTENT, 0, 0);
            for (const auto& assetPath : s->assetPaths)
            {
                const std::wstring item = ToWide(std::filesystem::path(assetPath).filename().string());
                SendMessageW(s->assetsList, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(item.c_str()));
            }
            if (!s->assetPaths.empty())
            {
                SendMessageW(s->assetsList, LB_SETCURSEL, 0, 0);
            }
        }

        void Accept(HWND hwnd, State* s)
        {
            const int sceneIdx = static_cast<int>(SendMessageW(s->scenesList, LB_GETCURSEL, 0, 0));
            const int assetIdx = static_cast<int>(SendMessageW(s->assetsList, LB_GETCURSEL, 0, 0));
            if (sceneIdx < 0 || sceneIdx >= static_cast<int>(s->scenePaths.size()))
            {
                return;
            }
            if (assetIdx < 0 || assetIdx >= static_cast<int>(s->assetPaths.size()))
            {
                return;
            }

            s->selectedScene = s->scenePaths[static_cast<size_t>(sceneIdx)];
            s->selectedAsset = std::filesystem::path(s->assetPaths[static_cast<size_t>(assetIdx)]).filename().string();
            s->accepted = true;
            s->done = true;
            DestroyWindow(hwnd);
        }

        LRESULT CALLBACK Proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
        {
            auto* s = reinterpret_cast<State*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
            switch (msg)
            {
            case WM_NCCREATE:
            {
                const auto* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
                auto* initState = reinterpret_cast<State*>(cs->lpCreateParams);
                SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(initState));
                return DefWindowProcW(hwnd, msg, wParam, lParam);
            }
            case WM_CREATE:
            {
                CreateWindowExW(0, L"STATIC", L"Scenes", WS_CHILD | WS_VISIBLE, 12, 8, 120, 20, hwnd, nullptr, nullptr, nullptr);
                s->scenesList = CreateWindowExW(
                    WS_EX_CLIENTEDGE, L"LISTBOX", nullptr,
                    WS_CHILD | WS_VISIBLE | LBS_NOTIFY | WS_VSCROLL | WS_BORDER,
                    12, 30, 250, 180, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kScenesListId)), nullptr, nullptr);

                CreateWindowExW(0, L"STATIC", L"Assets", WS_CHILD | WS_VISIBLE, 280, 8, 120, 20, hwnd, nullptr, nullptr, nullptr);
                s->assetsList = CreateWindowExW(
                    WS_EX_CLIENTEDGE, L"LISTBOX", nullptr,
                    WS_CHILD | WS_VISIBLE | LBS_NOTIFY | WS_VSCROLL | WS_BORDER,
                    280, 30, 250, 180, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kAssetsListId)), nullptr, nullptr);

                CreateWindowExW(0, L"STATIC", L"New Scene Name:", WS_CHILD | WS_VISIBLE, 12, 222, 100, 20, hwnd, nullptr, nullptr, nullptr);
                s->sceneEdit = CreateWindowExW(
                    WS_EX_CLIENTEDGE, L"EDIT", L"new.scene",
                    WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                    114, 220, 148, 24, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kSceneNameEditId)), nullptr, nullptr);

                CreateWindowExW(0, L"BUTTON", L"New Scene", WS_CHILD | WS_VISIBLE, 12, 254, 80, 28, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kNewSceneBtnId)), nullptr, nullptr);
                CreateWindowExW(0, L"BUTTON", L"Delete Scene", WS_CHILD | WS_VISIBLE, 96, 254, 90, 28, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kDeleteSceneBtnId)), nullptr, nullptr);
                CreateWindowExW(0, L"BUTTON", L"Open Scene", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 358, 254, 80, 28, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kOpenBtnId)), nullptr, nullptr);
                CreateWindowExW(0, L"BUTTON", L"Cancel", WS_CHILD | WS_VISIBLE, 450, 254, 80, 28, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kCancelBtnId)), nullptr, nullptr);

                RefreshScenes(s);
                RefreshAssets(s);
                return 0;
            }
            case WM_COMMAND:
            {
                const int id = LOWORD(wParam);
                const int code = HIWORD(wParam);
                if ((id == kScenesListId || id == kAssetsListId) && code == LBN_DBLCLK)
                {
                    Accept(hwnd, s);
                    return 0;
                }
                if (id == kNewSceneBtnId && code == BN_CLICKED)
                {
                    std::string name = ReadEdit(s->sceneEdit);
                    if (name.empty())
                    {
                        name = "new.scene";
                    }
                    if (name.find(".json") == std::string::npos)
                    {
                        name += ".json";
                    }
                    const std::string path = (std::filesystem::path(s->projectPath) / "scenes" / name).string();
                    std::vector<assets::SceneObjectSpec> empty;
                    std::string err;
                    assets::SceneRepository::SaveScene(path, empty, &err);
                    RefreshScenes(s);
                    return 0;
                }
                if (id == kDeleteSceneBtnId && code == BN_CLICKED)
                {
                    const int idx = static_cast<int>(SendMessageW(s->scenesList, LB_GETCURSEL, 0, 0));
                    if (idx < 0 || idx >= static_cast<int>(s->scenePaths.size()))
                    {
                        return 0;
                    }
                    std::error_code ec;
                    std::filesystem::remove(std::filesystem::path(s->scenePaths[static_cast<size_t>(idx)]), ec);
                    RefreshScenes(s);
                    return 0;
                }
                if (id == kOpenBtnId && code == BN_CLICKED)
                {
                    Accept(hwnd, s);
                    return 0;
                }
                if (id == kCancelBtnId && code == BN_CLICKED)
                {
                    s->done = true;
                    DestroyWindow(hwnd);
                    return 0;
                }
                break;
            }
            case WM_CLOSE:
                s->done = true;
                DestroyWindow(hwnd);
                return 0;
            }
            return DefWindowProcW(hwnd, msg, wParam, lParam);
        }
    } // namespace

    bool SceneHubWindow::ShowModal(
        HINSTANCE instance,
        const std::string& projectPath,
        const std::string& modelsDirectory,
        std::string& outScenePath,
        std::string& outPlacementAsset)
    {
        LOG_METHOD("SceneHubWindow", "ShowModal");
        outScenePath.clear();
        outPlacementAsset.clear();

        const wchar_t* className = L"DX11SceneHub";
        WNDCLASSEXW wc{};
        wc.cbSize = sizeof(WNDCLASSEXW);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = Proc;
        wc.hInstance = instance;
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
        wc.lpszClassName = className;
        RegisterClassExW(&wc);

        State s{};
        s.projectPath = projectPath;
        s.modelsDir = modelsDirectory;

        HWND hwnd = CreateWindowExW(
            0, className, L"Scene Hub",
            WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
            CW_USEDEFAULT, CW_USEDEFAULT, 560, 330, nullptr, nullptr, instance, &s);
        if (!hwnd)
        {
            return false;
        }
        ShowWindow(hwnd, SW_SHOW);
        UpdateWindow(hwnd);

        MSG msg{};
        while (!s.done && GetMessageW(&msg, nullptr, 0, 0))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }

        if (s.accepted)
        {
            outScenePath = s.selectedScene;
            outPlacementAsset = s.selectedAsset;
            return true;
        }
        return false;
    }
}
