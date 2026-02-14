#include "editor/AssetPickerWindow.h"
#include "core/Logger.h"

#include <commdlg.h>
#include <filesystem>

#include "assets/AssetRegistry.h"
#include "editor/MaterialEditorWindow.h"
#include "editor/ModelImport.h"

namespace editor
{
    namespace
    {
        constexpr int kListId = 1001;
        constexpr int kLoadButtonId = 1002;
        constexpr int kCancelButtonId = 1003;
        constexpr int kMaterialsButtonId = 1004;
        constexpr int kImportButtonId = 1005;

        struct PickerState
        {
            HINSTANCE instance = nullptr;
            std::string modelsDirectory;
            std::string materialsPath;
            std::vector<std::string> assets;
            std::string selected;
            bool done = false;
            bool accepted = false;
            HWND listBox = nullptr;
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

        void RefreshAssets(PickerState* state)
        {
            assets::AssetRegistry registry;
            std::string err;
            registry.ScanGltf(state->modelsDirectory, &err);
            state->assets = registry.GetAssetPaths();

            SendMessageW(state->listBox, LB_RESETCONTENT, 0, 0);
            for (const std::string& path : state->assets)
            {
                const std::wstring item = ToWide(std::filesystem::path(path).filename().string());
                SendMessageW(state->listBox, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(item.c_str()));
            }
            if (!state->assets.empty())
            {
                SendMessageW(state->listBox, LB_SETCURSEL, 0, 0);
            }
        }

        void AcceptSelection(HWND hwnd, PickerState* state)
        {
            const int idx = static_cast<int>(SendMessageW(state->listBox, LB_GETCURSEL, 0, 0));
            if (idx == LB_ERR || idx < 0 || idx >= static_cast<int>(state->assets.size()))
            {
                return;
            }
            state->selected = state->assets[static_cast<size_t>(idx)];
            state->accepted = true;
            state->done = true;
            DestroyWindow(hwnd);
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

        void PaintHome(HWND hwnd)
        {
            PAINTSTRUCT ps{};
            HDC hdc = BeginPaint(hwnd, &ps);

            RECT rc{};
            GetClientRect(hwnd, &rc);

            HBRUSH bg = CreateSolidBrush(RGB(45, 45, 45));
            FillRect(hdc, &rc, bg);
            DeleteObject(bg);

            RECT leftPanel{0, 0, 285, rc.bottom};
            HBRUSH leftBrush = CreateSolidBrush(RGB(56, 56, 56));
            FillRect(hdc, &leftPanel, leftBrush);
            DeleteObject(leftBrush);

            RECT accent{0, 0, rc.right, 4};
            HBRUSH accentBrush = CreateSolidBrush(RGB(232, 138, 38));
            FillRect(hdc, &accent, accentBrush);
            DeleteObject(accentBrush);

            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, RGB(245, 245, 245));
            HFONT titleFont = CreateFontW(28, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
            HFONT oldFont = static_cast<HFONT>(SelectObject(hdc, titleFont));
            TextOutW(hdc, 18, 18, L"DirectX Editor", 13);
            SelectObject(hdc, oldFont);
            DeleteObject(titleFont);

            SetTextColor(hdc, RGB(205, 205, 205));
            HFONT subFont = CreateFontW(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
            oldFont = static_cast<HFONT>(SelectObject(hdc, subFont));
            TextOutW(hdc, 18, 56, L"Blender-like start flow:", 24);
            TextOutW(hdc, 18, 78, L"1) Import/Open model", 21);
            TextOutW(hdc, 18, 100, L"2) Edit materials/shaders", 26);
            TextOutW(hdc, 18, 122, L"3) Load selected asset", 23);

            SetTextColor(hdc, RGB(225, 225, 225));
            HFONT secFont = CreateFontW(18, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
            SelectObject(hdc, secFont);
            TextOutW(hdc, 300, 16, L"Recent/Available Models", 23);
            SelectObject(hdc, oldFont);
            DeleteObject(secFont);

            SelectObject(hdc, oldFont);
            DeleteObject(subFont);

            EndPaint(hwnd, &ps);
        }

        LRESULT CALLBACK PickerWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
        {
            auto* state = reinterpret_cast<PickerState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

            switch (msg)
            {
            case WM_NCCREATE:
            {
                const auto* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
                auto* initState = reinterpret_cast<PickerState*>(cs->lpCreateParams);
                SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(initState));
                return DefWindowProcW(hwnd, msg, wParam, lParam);
            }
            case WM_CREATE:
            {
                state->listBox = CreateWindowExW(
                    WS_EX_CLIENTEDGE,
                    L"LISTBOX",
                    nullptr,
                    WS_CHILD | WS_VISIBLE | LBS_NOTIFY | WS_VSCROLL | WS_BORDER,
                    300,
                    46,
                    286,
                    320,
                    hwnd,
                    reinterpret_cast<HMENU>(static_cast<INT_PTR>(kListId)),
                    nullptr,
                    nullptr);

                CreateWindowExW(0, L"BUTTON", L"Load Selected Asset", WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 18, 168, 250, 36, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kLoadButtonId)), nullptr, nullptr);
                CreateWindowExW(0, L"BUTTON", L"Import Model...", WS_VISIBLE | WS_CHILD, 18, 214, 250, 36, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kImportButtonId)), nullptr, nullptr);
                CreateWindowExW(0, L"BUTTON", L"Edit Materials...", WS_VISIBLE | WS_CHILD, 18, 260, 250, 36, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kMaterialsButtonId)), nullptr, nullptr);
                CreateWindowExW(0, L"BUTTON", L"Cancel", WS_VISIBLE | WS_CHILD, 18, 306, 250, 36, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kCancelButtonId)), nullptr, nullptr);

                RefreshAssets(state);
                return 0;
            }
            case WM_PAINT:
                PaintHome(hwnd);
                return 0;

            case WM_COMMAND:
            {
                const int id = LOWORD(wParam);
                const int code = HIWORD(wParam);

                if (id == kListId && code == LBN_DBLCLK)
                {
                    AcceptSelection(hwnd, state);
                    return 0;
                }
                if (id == kLoadButtonId && code == BN_CLICKED)
                {
                    AcceptSelection(hwnd, state);
                    return 0;
                }
                if (id == kImportButtonId && code == BN_CLICKED)
                {
                    std::string source;
                    if (!OpenModelFileDialog(hwnd, source))
                    {
                        return 0;
                    }

                    std::string importedName;
                    std::string err;
                    if (!ImportModelWithDependencies(source, state->modelsDirectory, importedName, &err))
                    {
                        MessageBoxA(hwnd, err.c_str(), "Import Model Error", MB_OK | MB_ICONERROR);
                        return 0;
                    }

                    RefreshAssets(state);
                    MessageBoxA(hwnd, "Model imported to assets/models.", "Import", MB_OK | MB_ICONINFORMATION);
                    return 0;
                }
                if (id == kMaterialsButtonId && code == BN_CLICKED)
                {
                    MaterialEditorWindow::ShowModal(state->instance, state->materialsPath, state->modelsDirectory);
                    RefreshAssets(state);
                    return 0;
                }
                if (id == kCancelButtonId && code == BN_CLICKED)
                {
                    state->done = true;
                    state->accepted = false;
                    DestroyWindow(hwnd);
                    return 0;
                }
                break;
            }
            case WM_CLOSE:
                state->done = true;
                state->accepted = false;
                DestroyWindow(hwnd);
                return 0;
            case WM_DESTROY:
                return 0;
            }

            return DefWindowProcW(hwnd, msg, wParam, lParam);
        }
    } // namespace

    bool AssetPickerWindow::ShowModal(
        HINSTANCE instance,
        const std::string& modelsDirectory,
        const std::string& materialsJsonPath,
        std::string& outSelectedPath)
    {
        LOG_METHOD("AssetPickerWindow", "ShowModal");
        outSelectedPath.clear();

        const wchar_t* className = L"DX11EditorHomeWindow";
        WNDCLASSEXW wc{};
        wc.cbSize = sizeof(WNDCLASSEXW);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = PickerWndProc;
        wc.hInstance = instance;
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
        wc.lpszClassName = className;
        RegisterClassExW(&wc);

        PickerState state{};
        state.instance = instance;
        state.modelsDirectory = modelsDirectory;
        state.materialsPath = materialsJsonPath;

        HWND hwnd = CreateWindowExW(
            0,
            className,
            L"Editor Home",
            WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            620,
            430,
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

        if (state.accepted)
        {
            outSelectedPath = state.selected;
            return true;
        }
        return false;
    }
}
