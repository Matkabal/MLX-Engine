#pragma once

#include <windows.h>
#include <cstdint>

namespace core
{
    struct WindowConfig
    {
        const wchar_t* title = L"DX11 Study";
        uint32_t width = 1280;
        uint32_t height = 720;
    };

    // Engine-friendly callback surface:
    // Window owns Win32 details; renderer/game receive clean events.
    class WindowEvents
    {
    public:
        virtual ~WindowEvents() = default;
        virtual void OnResize(uint32_t width, uint32_t height) = 0;
        virtual void OnCloseRequested() = 0;
        virtual void OnMouseMove(int32_t x, int32_t y) = 0;
        virtual void OnMouseButton(bool leftDown) = 0;
        virtual void OnMouseWheel(float delta) { (void)delta; }
        virtual void OnBackToStartRequested() = 0;
        virtual void OnCommand(int id, int code) { (void)id; (void)code; }
        virtual void OnNotify(const NMHDR* hdr) { (void)hdr; }
    };

    class Window final
    {
    public:
        static constexpr int kBackToStartCommandId = 9001;

        Window() = default;
        ~Window() = default;

        Window(const Window&) = delete;
        Window& operator=(const Window&) = delete;

        bool Create(HINSTANCE instance, const WindowConfig& config, WindowEvents* events = nullptr);
        void Show(int cmdShow = SW_SHOW);
        bool PumpMessages(); // false when WM_QUIT is received

        HWND GetHandle() const { return hwnd_; }
        uint32_t GetWidth() const { return width_; }
        uint32_t GetHeight() const { return height_; }

    private:
        static LRESULT CALLBACK StaticWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
        LRESULT HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam);

        HINSTANCE instance_ = nullptr;
        HWND hwnd_ = nullptr;
        WindowEvents* events_ = nullptr;
        uint32_t width_ = 0;
        uint32_t height_ = 0;
    };
}
