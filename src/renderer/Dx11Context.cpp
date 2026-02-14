#include "renderer/Dx11Context.h"
#include "core/Logger.h"

namespace renderer
{
    bool Dx11Context::Initialize(HWND hwnd, uint32_t width, uint32_t height)
    {
        LOG_METHOD("Dx11Context", "Initialize");
        width_ = width;
        height_ = height;
        viewportX_ = 0;
        viewportY_ = 0;
        viewportWidth_ = width_;
        viewportHeight_ = height_;

        DXGI_SWAP_CHAIN_DESC scd{};
        scd.BufferDesc.Width = width_;
        scd.BufferDesc.Height = height_;
        scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        scd.BufferDesc.RefreshRate.Numerator = 60;
        scd.BufferDesc.RefreshRate.Denominator = 1;
        scd.SampleDesc.Count = 1;
        scd.SampleDesc.Quality = 0;
        scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        scd.BufferCount = 2;
        scd.OutputWindow = hwnd;
        scd.Windowed = TRUE;
        scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

        UINT createFlags = 0;
#if defined(_DEBUG)
        createFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

        D3D_FEATURE_LEVEL featureLevels[] = {D3D_FEATURE_LEVEL_11_0};
        D3D_FEATURE_LEVEL chosenLevel{};

        const HRESULT hr = D3D11CreateDeviceAndSwapChain(
            nullptr,
            D3D_DRIVER_TYPE_HARDWARE,
            nullptr,
            createFlags,
            featureLevels,
            ARRAYSIZE(featureLevels),
            D3D11_SDK_VERSION,
            &scd,
            swapChain_.GetAddressOf(),
            device_.GetAddressOf(),
            &chosenLevel,
            deviceContext_.GetAddressOf());
        ThrowIfFailed(hr, "Failed to create D3D11 device and swap chain.");

        CreateBackBufferView();
        UpdateViewport();
        return true;
    }

    void Dx11Context::Resize(uint32_t width, uint32_t height)
    {
        LOG_METHOD("Dx11Context", "Resize");
        width_ = width;
        height_ = height;
        viewportWidth_ = width_;
        viewportHeight_ = height_;

        if (!swapChain_ || width_ == 0 || height_ == 0)
        {
            return;
        }

        deviceContext_->OMSetRenderTargets(0, nullptr, nullptr);
        rtv_.Reset();

        const HRESULT hr = swapChain_->ResizeBuffers(0, width_, height_, DXGI_FORMAT_UNKNOWN, 0);
        ThrowIfFailed(hr, "Failed to resize swap chain buffers.");

        CreateBackBufferView();
        UpdateViewport();
    }

    void Dx11Context::SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
    {
        viewportX_ = x;
        viewportY_ = y;
        viewportWidth_ = width;
        viewportHeight_ = height;
        UpdateViewport();
    }

    bool Dx11Context::BeginFrame(const float clearColor[4])
    {
        LOG_METHOD("Dx11Context", "BeginFrame");
        if (!rtv_)
        {
            return false;
        }

        deviceContext_->OMSetRenderTargets(1, rtv_.GetAddressOf(), nullptr);
        deviceContext_->ClearRenderTargetView(rtv_.Get(), clearColor);
        deviceContext_->RSSetViewports(1, &viewport_);
        return true;
    }

    void Dx11Context::EndFrame()
    {
        LOG_METHOD("Dx11Context", "EndFrame");
        if (swapChain_)
        {
            swapChain_->Present(1, 0);
        }
    }

    void Dx11Context::CreateBackBufferView()
    {
        LOG_METHOD("Dx11Context", "CreateBackBufferView");
        Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer;
        HRESULT hr = swapChain_->GetBuffer(
            0,
            __uuidof(ID3D11Texture2D),
            reinterpret_cast<void**>(backBuffer.GetAddressOf()));
        ThrowIfFailed(hr, "Failed to get back buffer.");

        hr = device_->CreateRenderTargetView(backBuffer.Get(), nullptr, rtv_.GetAddressOf());
        ThrowIfFailed(hr, "Failed to create render target view.");
    }

    void Dx11Context::UpdateViewport()
    {
        LOG_METHOD("Dx11Context", "UpdateViewport");
        viewport_.TopLeftX = static_cast<float>(viewportX_);
        viewport_.TopLeftY = static_cast<float>(viewportY_);
        viewport_.Width = static_cast<float>(viewportWidth_);
        viewport_.Height = static_cast<float>(viewportHeight_);
        viewport_.MinDepth = 0.0f;
        viewport_.MaxDepth = 1.0f;
    }

    void Dx11Context::ThrowIfFailed(HRESULT hr, const char* message)
    {
        LOG_METHOD("Dx11Context", "ThrowIfFailed");
        if (FAILED(hr))
        {
            MessageBoxA(nullptr, message, "DX11 Context Error", MB_ICONERROR | MB_OK);
            ExitProcess(static_cast<UINT>(hr));
        }
    }
}
