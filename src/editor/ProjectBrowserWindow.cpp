#include "editor/ProjectBrowserWindow.h"

#include <filesystem>
#include <vector>

#include "core/Logger.h"

namespace editor
{
    namespace
    {
        constexpr int kProjectsListId = 3001;
        constexpr int kNameEditId = 3002;
        constexpr int kCreateBtnId = 3003;
        constexpr int kDeleteBtnId = 3004;
        constexpr int kOpenBtnId = 3005;
        constexpr int kCancelBtnId = 3006;

        struct State
        {
            std::string root;
            std::vector<std::string> projects;
            bool done = false;
            bool accepted = false;
            std::string selectedPath;
            HWND list = nullptr;
            HWND nameEdit = nullptr;
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

        void RefreshProjects(State* s)
        {
            namespace fs = std::filesystem;
            s->projects.clear();
            std::error_code ec;
            fs::create_directories(s->root, ec);
            for (const auto& entry : fs::directory_iterator(fs::path(s->root), ec))
            {
                if (entry.is_directory())
                {
                    s->projects.push_back(entry.path().string());
                }
            }

            SendMessageW(s->list, LB_RESETCONTENT, 0, 0);
            for (const auto& projectPath : s->projects)
            {
                const std::wstring name = ToWide(std::filesystem::path(projectPath).filename().string());
                SendMessageW(s->list, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(name.c_str()));
            }
            if (!s->projects.empty())
            {
                SendMessageW(s->list, LB_SETCURSEL, 0, 0);
            }
        }

        void AcceptSelected(HWND hwnd, State* s)
        {
            const int idx = static_cast<int>(SendMessageW(s->list, LB_GETCURSEL, 0, 0));
            if (idx < 0 || idx >= static_cast<int>(s->projects.size()))
            {
                return;
            }
            s->selectedPath = s->projects[static_cast<size_t>(idx)];
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
                s->list = CreateWindowExW(
                    WS_EX_CLIENTEDGE, L"LISTBOX", nullptr,
                    WS_CHILD | WS_VISIBLE | LBS_NOTIFY | WS_VSCROLL | WS_BORDER,
                    12, 12, 360, 210, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kProjectsListId)), nullptr, nullptr);
                CreateWindowExW(0, L"STATIC", L"Project Name:", WS_CHILD | WS_VISIBLE, 12, 234, 90, 20, hwnd, nullptr, nullptr, nullptr);
                s->nameEdit = CreateWindowExW(
                    WS_EX_CLIENTEDGE, L"EDIT", nullptr,
                    WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                    104, 231, 268, 24, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kNameEditId)), nullptr, nullptr);
                CreateWindowExW(0, L"BUTTON", L"Create", WS_CHILD | WS_VISIBLE, 12, 268, 80, 28, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kCreateBtnId)), nullptr, nullptr);
                CreateWindowExW(0, L"BUTTON", L"Delete", WS_CHILD | WS_VISIBLE, 96, 268, 80, 28, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kDeleteBtnId)), nullptr, nullptr);
                CreateWindowExW(0, L"BUTTON", L"Open", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 208, 268, 80, 28, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kOpenBtnId)), nullptr, nullptr);
                CreateWindowExW(0, L"BUTTON", L"Cancel", WS_CHILD | WS_VISIBLE, 292, 268, 80, 28, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kCancelBtnId)), nullptr, nullptr);
                RefreshProjects(s);
                return 0;
            }
            case WM_COMMAND:
            {
                const int id = LOWORD(wParam);
                const int code = HIWORD(wParam);
                if (id == kProjectsListId && code == LBN_DBLCLK)
                {
                    AcceptSelected(hwnd, s);
                    return 0;
                }
                if (id == kCreateBtnId && code == BN_CLICKED)
                {
                    const std::string name = ReadEdit(s->nameEdit);
                    if (name.empty())
                    {
                        MessageBoxA(hwnd, "Project name is required.", "Projects", MB_OK | MB_ICONWARNING);
                        return 0;
                    }
                    std::error_code ec;
                    std::filesystem::create_directories(std::filesystem::path(s->root) / name, ec);
                    RefreshProjects(s);
                    return 0;
                }
                if (id == kDeleteBtnId && code == BN_CLICKED)
                {
                    const int idx = static_cast<int>(SendMessageW(s->list, LB_GETCURSEL, 0, 0));
                    if (idx < 0 || idx >= static_cast<int>(s->projects.size()))
                    {
                        return 0;
                    }
                    std::error_code ec;
                    std::filesystem::remove_all(std::filesystem::path(s->projects[static_cast<size_t>(idx)]), ec);
                    RefreshProjects(s);
                    return 0;
                }
                if (id == kOpenBtnId && code == BN_CLICKED)
                {
                    AcceptSelected(hwnd, s);
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

    bool ProjectBrowserWindow::ShowModal(HINSTANCE instance, const std::string& projectsRoot, std::string& outProjectPath)
    {
        LOG_METHOD("ProjectBrowserWindow", "ShowModal");
        outProjectPath.clear();

        const wchar_t* className = L"DX11ProjectBrowser";
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
        s.root = projectsRoot;

        HWND hwnd = CreateWindowExW(
            0, className, L"Projects",
            WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
            CW_USEDEFAULT, CW_USEDEFAULT, 400, 350, nullptr, nullptr, instance, &s);
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
            outProjectPath = s.selectedPath;
            return true;
        }
        return false;
    }
}
