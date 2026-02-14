#pragma once

#include <cstdint>

#include <d3d11.h>
#include <dxgi.h>
#include <windows.h>
#include <wrl/client.h>

namespace renderer
{
    class Dx11Context
    {
    public:
        bool Initialize(HWND hwnd, uint32_t width, uint32_t height);
        void Resize(uint32_t width, uint32_t height);
        void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height);
        bool BeginFrame(const float clearColor[4]);
        void EndFrame();

        ID3D11Device* GetDevice() const { return device_.Get(); }
        ID3D11DeviceContext* GetDeviceContext() const { return deviceContext_.Get(); }

    private:
        void CreateBackBufferView();
        void UpdateViewport();
        static void ThrowIfFailed(HRESULT hr, const char* message);

    private:
        uint32_t width_ = 0;
        uint32_t height_ = 0;
        uint32_t viewportX_ = 0;
        uint32_t viewportY_ = 0;
        uint32_t viewportWidth_ = 0;
        uint32_t viewportHeight_ = 0;
        Microsoft::WRL::ComPtr<ID3D11Device> device_;
        Microsoft::WRL::ComPtr<ID3D11DeviceContext> deviceContext_;
        Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain_;
        Microsoft::WRL::ComPtr<ID3D11RenderTargetView> rtv_;
        D3D11_VIEWPORT viewport_{};
    };
}
