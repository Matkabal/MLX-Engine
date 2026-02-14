#include "core/Window.h"
#include "core/Logger.h"
#include <windowsx.h>

namespace core
{
    bool Window::Create(HINSTANCE instance, const WindowConfig& config, WindowEvents* events)
    {
        LOG_METHOD("Window", "Create");
        instance_ = instance;
        events_ = events;
        width_ = config.width;
        height_ = config.height;

        const wchar_t* className = L"DX11StudyWindowClass";

        WNDCLASSEXW wc{};
        wc.cbSize = sizeof(WNDCLASSEXW);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = &Window::StaticWndProc;
        wc.hInstance = instance_;
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.lpszClassName = className;

        if (!RegisterClassExW(&wc))
        {
            if (GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
            {
                return false;
            }
        }

        const DWORD windowStyle = WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
        RECT rect{0, 0, static_cast<LONG>(width_), static_cast<LONG>(height_)};
        AdjustWindowRect(&rect, windowStyle, FALSE);

        hwnd_ = CreateWindowExW(
            0,
            className,
            config.title,
            windowStyle,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            rect.right - rect.left,
            rect.bottom - rect.top,
            nullptr,
            nullptr,
            instance_,
            this);

        return hwnd_ != nullptr;
    }

    void Window::Show(int cmdShow)
    {
        LOG_METHOD("Window", "Show");
        if (hwnd_)
        {
            ShowWindow(hwnd_, cmdShow);
            UpdateWindow(hwnd_);
        }
    }

    bool Window::PumpMessages()
    {
        LOG_METHOD("Window", "PumpMessages");
        MSG msg{};
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                return false;
            }

            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        return true;
    }

    LRESULT CALLBACK Window::StaticWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        LOG_METHOD("Window", "StaticWndProc");
        Window* self = reinterpret_cast<Window*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

        if (msg == WM_NCCREATE)
        {
            const auto* createStruct = reinterpret_cast<CREATESTRUCTW*>(lParam);
            self = reinterpret_cast<Window*>(createStruct->lpCreateParams);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
            if (self)
            {
                self->hwnd_ = hwnd;
            }
        }

        if (self)
        {
            return self->HandleMessage(msg, wParam, lParam);
        }

        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    LRESULT Window::HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam)
    {
        LOG_METHOD("Window", "HandleMessage");
        switch (msg)
        {
        case WM_SIZE:
            width_ = static_cast<uint32_t>(LOWORD(lParam));
            height_ = static_cast<uint32_t>(HIWORD(lParam));
            if (events_)
            {
                events_->OnResize(width_, height_);
            }
            return 0;

        case WM_CLOSE:
            if (events_)
            {
                events_->OnCloseRequested();
            }
            DestroyWindow(hwnd_);
            return 0;

        case WM_MOUSEMOVE:
            if (events_)
            {
                events_->OnMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            }
            return 0;

        case WM_LBUTTONDOWN:
            if (events_)
            {
                events_->OnMouseButton(true);
            }
            return 0;

        case WM_LBUTTONUP:
            if (events_)
            {
                events_->OnMouseButton(false);
            }
            return 0;

        case WM_MOUSEWHEEL:
            if (events_)
            {
                const short wheelDelta = GET_WHEEL_DELTA_WPARAM(wParam);
                events_->OnMouseWheel(static_cast<float>(wheelDelta) / static_cast<float>(WHEEL_DELTA));
            }
            return 0;

        case WM_COMMAND:
            if (events_)
            {
                events_->OnCommand(LOWORD(wParam), HIWORD(wParam));
            }
            if (LOWORD(wParam) == Window::kBackToStartCommandId && events_)
            {
                events_->OnBackToStartRequested();
                return 0;
            }
            break;

        case WM_NOTIFY:
            if (events_)
            {
                const NMHDR* hdr = reinterpret_cast<const NMHDR*>(lParam);
                events_->OnNotify(hdr);
            }
            return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        default:
            return DefWindowProcW(hwnd_, msg, wParam, lParam);
        }

        return DefWindowProcW(hwnd_, msg, wParam, lParam);
    }
}
