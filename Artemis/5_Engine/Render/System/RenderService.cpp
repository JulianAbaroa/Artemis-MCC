module;

#include <d3d11.h>
#include <dxgi.h>
#include <wrl/client.h>

module Engine.Render.System;
import :RenderService;
import :GpuStateGuard;
import std;

namespace
{
    using Microsoft::WRL::ComPtr;

    using Engine::Render::Type::ColorTargetInfo;
    using Engine::Render::Type::DepthStencilStateInfo;
    using Engine::Render::Type::DepthTargetInfo;
    using Engine::Render::Type::FirstPersonCaller;
    using Engine::Render::Type::FirstPersonSample;
    using Engine::Render::Type::FormatName;
    using Engine::Render::Type::HudHookInfo;
    using Engine::Render::Type::RasterizerStateInfo;
    using Engine::Render::Type::ViewportInfo;

    using Engine::Render::Type::k_FirstPersonCallerCount;
    using Engine::Render::Type::k_FirstPersonCallerRanges;

    using std::chrono::steady_clock;
    using std::chrono::microseconds;

    // Reads the texture, the view and the identity of a render target. The caller owns the view.
    auto MeasureColorTarget(ID3D11RenderTargetView* target, ID3D11RenderTargetView* backBufferView, ID3D11Device* storeDevice)
        -> ColorTargetInfo
    {
        ColorTargetInfo info{};
        if (target == nullptr) return info;

        D3D11_RENDER_TARGET_VIEW_DESC viewDesc{};
        target->GetDesc(&viewDesc);

        ComPtr<ID3D11Resource> resource{};
        target->GetResource(resource.GetAddressOf());

        ComPtr<ID3D11Texture2D> texture{};
        if (resource == nullptr || FAILED(resource.As(&texture))) return info;

        D3D11_TEXTURE2D_DESC textureDesc{};
        texture->GetDesc(&textureDesc);

        info.HasTarget = true;
        info.Width = textureDesc.Width;
        info.Height = textureDesc.Height;
        info.ResourceFormat = static_cast<std::uint32_t>(textureDesc.Format);
        info.ViewFormat = static_cast<std::uint32_t>(viewDesc.Format);

        ComPtr<ID3D11Resource> backBuffer{};
        if (backBufferView != nullptr) backBufferView->GetResource(backBuffer.GetAddressOf());

        info.IsBackBuffer = backBuffer != nullptr && backBuffer.Get() == resource.Get();

        ComPtr<ID3D11Device> targetDevice{};
        target->GetDevice(targetDevice.GetAddressOf());
        info.IsSameDevice = targetDevice != nullptr && targetDevice.Get() == storeDevice;
        return info;
    }

    // Reads the texture and the view of a depth stencil view. The caller owns the view.
    auto MeasureDepthTarget(ID3D11DepthStencilView* view) -> DepthTargetInfo
    {
        DepthTargetInfo info{};
        if (view == nullptr) return info;

        D3D11_DEPTH_STENCIL_VIEW_DESC viewDesc{};
        view->GetDesc(&viewDesc);

        ComPtr<ID3D11Resource> resource{};
        view->GetResource(resource.GetAddressOf());

        info.HasDepth = true;
        info.Resource = reinterpret_cast<std::uintptr_t>(resource.Get());
        info.ViewFormat = static_cast<std::uint32_t>(viewDesc.Format);
        info.ViewFlags = viewDesc.Flags;

        ComPtr<ID3D11Texture2D> texture{};
        if (resource != nullptr && SUCCEEDED(resource.As(&texture)))
        {
            D3D11_TEXTURE2D_DESC textureDesc{};
            texture->GetDesc(&textureDesc);

            info.Width = textureDesc.Width;
            info.Height = textureDesc.Height;
            info.ResourceFormat = static_cast<std::uint32_t>(textureDesc.Format);
        }

        return info;
    }

    auto FormatText(std::uint32_t format) -> std::string
    {
        const char* name{ FormatName(format) };
        return name != nullptr ? std::string{ name } : std::format("fmt {}", format);
    }

    auto ComparisonName(std::uint32_t func) -> std::string
    {
        switch (func)
        {
        case D3D11_COMPARISON_NEVER:            return "NEVER";
        case D3D11_COMPARISON_LESS:             return "LESS";
        case D3D11_COMPARISON_EQUAL:            return "EQUAL";
        case D3D11_COMPARISON_LESS_EQUAL:       return "LESS_EQUAL";
        case D3D11_COMPARISON_GREATER:          return "GREATER";
        case D3D11_COMPARISON_NOT_EQUAL:        return "NOT_EQUAL";
        case D3D11_COMPARISON_GREATER_EQUAL:    return "GREATER_EQUAL";
        case D3D11_COMPARISON_ALWAYS:           return "ALWAYS";
        default:                                return std::format("{}", func);
        }
    }

    auto StencilOpName(std::uint32_t op) -> std::string
    {
        switch (op)
        {
        case D3D11_STENCIL_OP_KEEP:     return "KEEP";
        case D3D11_STENCIL_OP_ZERO:     return "ZERO";
        case D3D11_STENCIL_OP_REPLACE:  return "REPLACE";
        case D3D11_STENCIL_OP_INCR_SAT: return "INCR_SAT";
        case D3D11_STENCIL_OP_DECR_SAT: return "DECR_SAT";
        case D3D11_STENCIL_OP_INVERT:   return "INVERT";
        case D3D11_STENCIL_OP_INCR:     return "INCR";
        case D3D11_STENCIL_OP_DECR:     return "DECR";
        default:                        return std::format("{}", op);
        }
    }

    auto CullName(std::uint32_t mode) -> const char*
    {
        switch (mode)
        {
        case D3D11_CULL_NONE:   return "NONE";
        case D3D11_CULL_FRONT:  return "FRONT";
        case D3D11_CULL_BACK:   return "BACK";
        default:                return "?";
        }
    }

    auto DescribeColor(const ColorTargetInfo& color, std::uint32_t count) -> std::string
    {
        if (!color.HasTarget) return std::format("rtvs {}, rtv0 none", count);

        return std::format("rtvs {}, rtv0 {}x{} res {} view {} back buffer {}", count, color.Width, color.Height,
            FormatText(color.ResourceFormat), color.ViewFormat != 0 ? FormatText(color.ViewFormat) : std::string{ "same" },
            color.IsBackBuffer ? "yes" : "no");
    }

    auto DescribeDepth(const DepthTargetInfo& depth) -> std::string
    {
        if (!depth.HasDepth) return "dsv none";

        return std::format("dsv {}x{} res {} view {} flags {} resource {:#x}", depth.Width, depth.Height, FormatText(depth.ResourceFormat),
            FormatText(depth.ViewFormat), depth.ViewFlags, depth.Resource);
    }

    // return: The first byte of the function of that caller, 0 for Other.
    auto CallerRvaOf(FirstPersonCaller caller) -> std::uint32_t
    {
        for (const auto& range : k_FirstPersonCallerRanges)
        {
            if (range.Caller == caller) return range.Rva;
        }

        return 0;
    }

    auto DescribeFirstPerson(const FirstPersonSample& sample) -> std::string
    {
        const std::string caller{ sample.Caller == FirstPersonCaller::Other ? std::format("other rva {:#X}", sample.CallerRva)
            : std::format("{:#X}", CallerRvaOf(sample.Caller)) };

        if (!sample.HasSample) return std::format("caller {} calls {} (no sample)", caller, sample.CallsPerFrame);
        if (!sample.IsSameThread) return std::format("caller {} calls {} pass {} (other thread, no D3D read)", caller, sample.CallsPerFrame, sample.Pass);

        const ViewportInfo& viewport{ sample.Viewport };
        const DepthStencilStateInfo& state{ sample.DepthState };
        const RasterizerStateInfo& raster{ sample.Rasterizer };

        std::string depthState{};

        if (state.HasState)
        {
            depthState = std::format("depth {} write {} func {} stencil {} rmask {:#X} wmask {:#X} ref {} front func {} pass {} fail {} dfail {}",
                state.IsDepthEnabled ? "on" : "off", state.DepthWriteMask == D3D11_DEPTH_WRITE_MASK_ALL ? "ALL" : "ZERO",
                ComparisonName(state.DepthFunc), state.IsStencilEnabled ? "on" : "off", state.StencilReadMask, state.StencilWriteMask,
                state.StencilRef, ComparisonName(state.FrontFunc), StencilOpName(state.FrontPassOp), StencilOpName(state.FrontFailOp),
                StencilOpName(state.FrontDepthFailOp));
        }
        else
        {
            depthState = std::format("depth state default (ref {})", state.StencilRef);
        }

        const std::string rasterizer{ raster.HasState ? std::format("bias {} slope {} clip {} cull {}", raster.DepthBias,
            raster.SlopeScaledDepthBias, raster.IsDepthClipEnabled ? "on" : "off", CullName(raster.CullMode)) : std::string{ "default" } };

        return std::format("caller {} calls {} pass {} | {} | {} | viewport {},{} {}x{} depth {}-{} | {} | rasterizer {}", caller,
            sample.CallsPerFrame, sample.Pass, DescribeColor(sample.Color, sample.RenderTargetCount), DescribeDepth(sample.Depth),
            viewport.X, viewport.Y, viewport.Width, viewport.Height, viewport.MinDepth, viewport.MaxDepth, depthState, rasterizer);
    }

    auto DescribeChud(const HudHookInfo& chud, const FirstPersonSample& view) -> std::string
    {
        if (!chud.HasSample) return "chud draw no sample";
        if (!chud.IsSameThread) return std::format("chud draw calls {} (other thread, no D3D read)", chud.CallsPerFrame);

        const std::string target{ chud.HasTarget ? std::format("rtv0 {}x{} view {} back buffer {}", chud.Width, chud.Height,
            FormatText(chud.Format), chud.IsBackBuffer ? "yes" : "no") : std::string{ "rtv0 none" } };

        std::string same{ "no view sample" };
        if (view.HasSample && view.IsSameThread && view.Depth.HasDepth && chud.Depth.HasDepth)
        {
            same = view.Depth.Resource == chud.Depth.Resource ? "yes" : "no";
        }
        else if (view.HasSample && view.IsSameThread && !chud.Depth.HasDepth)
        {
            same = "no dsv bound";
        }

        return std::format("chud draw calls {} | {} | {} | same dsv resource as 0x26EA78: {}", chud.CallsPerFrame, target,
            DescribeDepth(chud.Depth), same);
    }
}

namespace Engine::Render::System
{
    auto RenderService::OnInitialized(FrameHandler handler) -> void
    {
        m_OnInitialized.push_back(std::move(handler));
    }

    auto RenderService::OnFrame(FrameHandler handler) -> void
    {
        m_OnFrame.push_back(std::move(handler));
    }

    auto RenderService::OnPostFrame(FrameHandler handler) -> void
    {
        m_OnPostFrame.push_back(std::move(handler));
    }

    auto RenderService::OnResize(FrameHandler handler) -> void
    {
        m_OnResize.push_back(std::move(handler));
    }

    auto RenderService::OnBeforeHud(BeforeHudHandler handler) -> void
    {
        m_OnBeforeHud.push_back(std::move(handler));
    }

    auto RenderService::OnShutdown(ShutdownCallback callback) -> void
    {
        m_OnShutdown.push_back(std::move(callback));
    }

    auto RenderService::PresentFrame(IDXGISwapChain* swapChain) -> void
    {
        if (m_RenderStore.IsResizing()) return;

        if (m_RenderStore.IsShutdownRequested())
        {
            this->Shutdown();
            return;
        }

        if (m_RenderStore.StepFirstPersonProbe()) this->ReportFirstPersonProbe();

        // A different swap chain than the bound one means the game created a new one.
        const bool isAnotherSwapChain = m_RenderStore.GetSwapChain() != swapChain;
        if (!m_RenderStore.IsReady() || isAnotherSwapChain)
        {
            this->Bind(swapChain);
            if (!m_RenderStore.IsReady()) return;
        }

        this->Raise(m_OnFrame, "OnFrame");
        this->Raise(m_OnPostFrame, "OnPostFrame");
    }

    auto RenderService::BeginResize() -> void
    {
        m_RenderStore.SetResizing(true);

        if (auto* context = m_RenderStore.GetContext())
        {
            context->OMSetRenderTargets(0, nullptr, nullptr);
        }

        m_RenderStore.ReleaseTargets();
    }

    auto RenderService::EndResize(IDXGISwapChain* swapChain, HRESULT result) -> void
    {
        if (SUCCEEDED(result) && !m_RenderStore.IsShutdownRequested())
        {
            this->Bind(swapChain);
        }

        m_RenderStore.SetResizing(false);
    }

    auto RenderService::Bind(IDXGISwapChain* swapChain) -> void
    {
        const bool isFirstBind = m_RenderStore.GetSwapChain() != swapChain;

        if (!this->Attach(swapChain)) return;

        this->Raise(isFirstBind ? m_OnInitialized : m_OnResize,
            isFirstBind ? "OnInitialized" : "OnResize");
    }

    auto RenderService::Attach(IDXGISwapChain* swapChain) -> bool
    {
        auto fail = [this](const char* reason)
        {
            if (!m_IsAttachFailed)
            {
                m_Log.Message("[RenderService] ERROR: {}", reason);
                m_IsAttachFailed = true;
            }

            return false;
        };

        ComPtr<ID3D11Device> device{};
        if (FAILED(swapChain->GetDevice(IID_PPV_ARGS(device.GetAddressOf()))))
        {
            return fail("The swap chain does not belong to a D3D11 device.");
        }

        ComPtr<ID3D11DeviceContext> context{};
        device->GetImmediateContext(context.GetAddressOf());

        DXGI_SWAP_CHAIN_DESC swapChainDesc{};
        if (FAILED(swapChain->GetDesc(&swapChainDesc)))
        {
            return fail("IDXGISwapChain::GetDesc failed.");
        }

        ComPtr<ID3D11Texture2D> backBuffer{};
        if (FAILED(swapChain->GetBuffer(0, IID_PPV_ARGS(backBuffer.GetAddressOf()))))
        {
            return fail("IDXGISwapChain::GetBuffer failed.");
        }

        D3D11_TEXTURE2D_DESC bufferDesc{};
        backBuffer->GetDesc(&bufferDesc);

        ComPtr<ID3D11RenderTargetView> renderTarget{};
        if (FAILED(device->CreateRenderTargetView(backBuffer.Get(), nullptr,
            renderTarget.GetAddressOf())))
        {
            return fail("CreateRenderTargetView failed.");
        }

        // Depth buffer with the size and samples of the back buffer.
        D3D11_TEXTURE2D_DESC depthDesc{};
        depthDesc.Width = bufferDesc.Width;
        depthDesc.Height = bufferDesc.Height;
        depthDesc.MipLevels = 1;
        depthDesc.ArraySize = 1;
        depthDesc.Format = DXGI_FORMAT_R32G8X24_TYPELESS;
        depthDesc.SampleDesc = bufferDesc.SampleDesc;
        depthDesc.Usage = D3D11_USAGE_DEFAULT;
        depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

        ComPtr<ID3D11DepthStencilView> depthStencil{};
        ComPtr<ID3D11Texture2D> depthTexture{};

        if (SUCCEEDED(device->CreateTexture2D(&depthDesc, nullptr,
            depthTexture.GetAddressOf())))
        {
            D3D11_DEPTH_STENCIL_VIEW_DESC viewDesc{};
            viewDesc.Format = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
            viewDesc.ViewDimension = bufferDesc.SampleDesc.Count > 1 ?
                D3D11_DSV_DIMENSION_TEXTURE2DMS : D3D11_DSV_DIMENSION_TEXTURE2D;

            if (FAILED(device->CreateDepthStencilView(depthTexture.Get(),
                &viewDesc, depthStencil.GetAddressOf())))
            {
                m_Log.Message("[RenderService] ERROR:"
                    " CreateDepthStencilView failed.");
            }
        }
        else
        {
            m_Log.Message("[RenderService] ERROR:"
                " Depth CreateTexture2D failed.");
        }

        m_RenderStore.SetDevice(std::move(device), std::move(context),
            swapChainDesc.OutputWindow, swapChain);

        m_RenderStore.SetTargets(std::move(renderTarget), std::move(depthStencil),
            bufferDesc.Width, bufferDesc.Height);

        m_IsAttachFailed = false;
        return true;
    }

    auto RenderService::MakeContext() const -> FrameContext
    {
        FrameContext frame{};
        frame.Device = m_RenderStore.GetDevice();
        frame.Context = m_RenderStore.GetContext();
        frame.RenderTarget = m_RenderStore.GetRenderTarget();
        frame.DepthStencil = m_RenderStore.GetDepthStencil();
        frame.Window = m_RenderStore.GetWindow();
        frame.Width = m_RenderStore.GetWidth();
        frame.Height = m_RenderStore.GetHeight();
        return frame;
    }

    auto RenderService::Raise(const std::vector<FrameHandler>& handlers,
        const char* stage) -> void
    {
        const FrameContext frame = this->MakeContext();

        for (const auto& handler : handlers)
        {
            try
            {
                handler(frame);
            }
            catch (...)
            {
                this->ReportHandlerError(stage);
            }
        }
    }

    auto RenderService::ReportHandlerError(const char* stage) noexcept -> void
    {
        if (m_HandlerErrors.fetch_add(1) >= 5) return;

        try
        {
            m_Log.Message("[RenderService] ERROR: A {} handler threw an exception.", stage);
        }
        catch (...)
        {
        }
    }

    auto RenderService::RequestShutdown() -> void
    {
        m_RenderStore.RequestShutdown();
    }

    auto RenderService::WaitForShutdown(milliseconds timeout) -> bool
    {
        const auto deadline = steady_clock::now() + timeout;

        while (!m_RenderStore.IsShutdownDone())
        {
            if (steady_clock::now() >= deadline) return false;
            std::this_thread::sleep_for(milliseconds{ 1 });
        }

        return true;
    }

    auto RenderService::FindBeforeHudBlocker(const HudHookInfo& sample, bool isReady, std::uint32_t width, std::uint32_t height)
        -> std::optional<BeforeHudStatus>
    {
        if (!sample.IsSameThread) return BeforeHudStatus::OtherThread;
        if (!isReady) return BeforeHudStatus::NotReady;
        if (!sample.HasTarget || width == 0 || height == 0) return BeforeHudStatus::TargetMismatch;
        if (!sample.IsSameDevice) return BeforeHudStatus::OtherDevice;
        if (!sample.IsBackBuffer) return BeforeHudStatus::NotBackBuffer;
        if (sample.Width != width || sample.Height != height) return BeforeHudStatus::TargetMismatch;

        const bool isWholeTarget{ sample.HasViewport && sample.ViewportX == 0.0f && sample.ViewportY == 0.0f &&
            sample.ViewportWidth == static_cast<float>(width) && sample.ViewportHeight == static_cast<float>(height) };
        if (!isWholeTarget) return BeforeHudStatus::SplitViewport;

        return std::nullopt;
    }

    auto RenderService::OnHudDraw() -> void
    {
        try
        {
            this->HandleHudDraw();
        }
        catch (...)
        {
            m_RenderStore.CountHudFailure();
        }
    }

    auto RenderService::HandleHudDraw() -> void
    {
        m_RenderStore.CountHudCall();

        const HudHookInfo sample{ this->MeasureHud() };

        if (m_RenderStore.IsGameHudHidden())
        {
            m_RenderStore.SetBeforeHudStatus(BeforeHudStatus::HudHidden);
            return;
        }

        // The handlers run once per Present frame, whatever the number of HUD draws.
        if (m_RenderStore.IsBeforeHudMarked()) return;

        const bool isReady{ m_RenderStore.IsReady() && !m_RenderStore.IsResizing() && !m_RenderStore.IsShutdownRequested() };

        if (const auto blocker{ this->FindBeforeHudBlocker(sample, isReady, m_RenderStore.GetWidth(), m_RenderStore.GetHeight()) })
        {
            m_RenderStore.SetBeforeHudStatus(*blocker);
            return;
        }

        ID3D11DeviceContext* context{ m_RenderStore.GetContext() };

        ComPtr<ID3D11RenderTargetView> target{};
        if (context != nullptr) context->OMGetRenderTargets(1, target.GetAddressOf(), nullptr);

        if (context == nullptr || target == nullptr)
        {
            m_RenderStore.SetBeforeHudStatus(BeforeHudStatus::NotReady);
            return;
        }

        FrameContext frame{ this->MakeContext() };
        frame.RenderTarget = target.Get();
        frame.Width = sample.Width;
        frame.Height = sample.Height;

        bool isDrawn{ false };
        bool isFailed{ false };

        {
            GpuStateGuard guard{ context };

            for (const auto& handler : m_OnBeforeHud)
            {
                try
                {
                    if (handler(frame)) isDrawn = true;
                }
                catch (...)
                {
                    // The handler may have drawn part of the frame: it counts as handled, so Present does not draw it again.
                    isFailed = true;
                    this->ReportHandlerError("OnBeforeHud");
                }
            }
        }

        if (isDrawn || isFailed) m_RenderStore.MarkBeforeHud();
        m_RenderStore.SetBeforeHudStatus(isFailed ? BeforeHudStatus::Failed : isDrawn ? BeforeHudStatus::Drawn : BeforeHudStatus::Declined);
    }

    auto RenderService::MeasureHud() -> HudHookInfo
    {
        HudHookInfo sample{};
        sample.HasSample = true;

        const std::uint32_t presentThread{ m_RenderStore.GetPresentThread() };
        sample.IsSameThread = presentThread != 0 && presentThread == GetCurrentThreadId();

        if (!sample.IsSameThread || m_RenderStore.IsResizing() || !m_RenderStore.IsReady())
        {
            m_RenderStore.SetHudSample(sample);
            return sample;
        }

        ID3D11DeviceContext* context{ m_RenderStore.GetContext() };

        if (context == nullptr)
        {
            m_RenderStore.SetHudSample(sample);
            return sample;
        }

        ComPtr<ID3D11RenderTargetView> target{};
        ComPtr<ID3D11DepthStencilView> depth{};
        context->OMGetRenderTargets(1, target.GetAddressOf(), depth.GetAddressOf());

        const ColorTargetInfo color{ MeasureColorTarget(target.Get(), m_RenderStore.GetRenderTarget(), m_RenderStore.GetDevice()) };

        if (color.HasTarget)
        {
            sample.HasTarget = true;
            sample.Width = color.Width;
            sample.Height = color.Height;
            sample.Format = color.ViewFormat != DXGI_FORMAT_UNKNOWN ? color.ViewFormat : color.ResourceFormat;
            sample.IsBackBuffer = color.IsBackBuffer;
            sample.IsSameDevice = color.IsSameDevice;
        }

        sample.Depth = MeasureDepthTarget(depth.Get());

        UINT viewportCount{ 1 };
        D3D11_VIEWPORT viewport{};
        context->RSGetViewports(&viewportCount, &viewport);

        if (viewportCount >= 1 && viewport.Width > 0.0f)
        {
            sample.HasViewport = true;
            sample.ViewportX = viewport.TopLeftX;
            sample.ViewportY = viewport.TopLeftY;
            sample.ViewportWidth = viewport.Width;
            sample.ViewportHeight = viewport.Height;
        }

        m_RenderStore.SetHudSample(sample);
        return sample;
    }

    auto RenderService::MeasureFirstPerson(FirstPersonCaller caller, std::uint32_t callerRva, std::uint8_t pass) -> void
    {
        FirstPersonSample sample{};
        sample.HasSample = true;
        sample.Caller = caller;
        sample.CallerRva = callerRva;
        sample.Pass = pass;

        const std::uint32_t presentThread{ m_RenderStore.GetPresentThread() };
        sample.IsSameThread = presentThread != 0 && presentThread == GetCurrentThreadId();

        ID3D11DeviceContext* context{ sample.IsSameThread && !m_RenderStore.IsResizing() && m_RenderStore.IsReady()
            ? m_RenderStore.GetContext() : nullptr };

        if (context == nullptr)
        {
            // Another thread, resizing or not ready: nothing of D3D is touched and the sample only says so.
            m_RenderStore.SetFirstPersonSample(sample);
            return;
        }

        std::array<ID3D11RenderTargetView*, D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT> views{};
        ComPtr<ID3D11DepthStencilView> depth{};
        context->OMGetRenderTargets(static_cast<UINT>(views.size()), views.data(), depth.GetAddressOf());

        ComPtr<ID3D11RenderTargetView> first{};

        for (std::size_t i{ 0 }; i < views.size(); ++i)
        {
            if (views[i] == nullptr) continue;

            ++sample.RenderTargetCount;

            if (i == 0) first.Attach(views[i]);
            else views[i]->Release();
        }

        sample.Color = MeasureColorTarget(first.Get(), m_RenderStore.GetRenderTarget(), m_RenderStore.GetDevice());
        sample.Depth = MeasureDepthTarget(depth.Get());

        UINT viewportCount{ 1 };
        D3D11_VIEWPORT viewport{};
        context->RSGetViewports(&viewportCount, &viewport);

        if (viewportCount >= 1)
        {
            sample.Viewport = { .HasViewport = true, .X = viewport.TopLeftX, .Y = viewport.TopLeftY, .Width = viewport.Width,
                .Height = viewport.Height, .MinDepth = viewport.MinDepth, .MaxDepth = viewport.MaxDepth };
        }

        ComPtr<ID3D11DepthStencilState> depthState{};
        UINT stencilRef{ 0 };
        context->OMGetDepthStencilState(depthState.GetAddressOf(), &stencilRef);
        sample.DepthState.StencilRef = stencilRef;

        if (depthState != nullptr)
        {
            D3D11_DEPTH_STENCIL_DESC desc{};
            depthState->GetDesc(&desc);

            sample.DepthState.HasState = true;
            sample.DepthState.IsDepthEnabled = desc.DepthEnable != FALSE;
            sample.DepthState.IsStencilEnabled = desc.StencilEnable != FALSE;
            sample.DepthState.DepthWriteMask = desc.DepthWriteMask;
            sample.DepthState.DepthFunc = desc.DepthFunc;
            sample.DepthState.StencilReadMask = desc.StencilReadMask;
            sample.DepthState.StencilWriteMask = desc.StencilWriteMask;
            sample.DepthState.FrontFunc = desc.FrontFace.StencilFunc;
            sample.DepthState.FrontPassOp = desc.FrontFace.StencilPassOp;
            sample.DepthState.FrontFailOp = desc.FrontFace.StencilFailOp;
            sample.DepthState.FrontDepthFailOp = desc.FrontFace.StencilDepthFailOp;
        }

        ComPtr<ID3D11RasterizerState> rasterizer{};
        context->RSGetState(rasterizer.GetAddressOf());

        if (rasterizer != nullptr)
        {
            D3D11_RASTERIZER_DESC desc{};
            rasterizer->GetDesc(&desc);

            sample.Rasterizer.HasState = true;
            sample.Rasterizer.IsDepthClipEnabled = desc.DepthClipEnable != FALSE;
            sample.Rasterizer.DepthBias = desc.DepthBias;
            sample.Rasterizer.SlopeScaledDepthBias = desc.SlopeScaledDepthBias;
            sample.Rasterizer.CullMode = desc.CullMode;
        }

        m_RenderStore.SetFirstPersonSample(sample);
    }

    auto RenderService::ReportFirstPersonProbe() -> void
    {
        for (std::size_t i{ 0 }; i < k_FirstPersonCallerCount; ++i)
        {
            const FirstPersonSample sample{ m_RenderStore.GetFirstPersonSample(static_cast<FirstPersonCaller>(i)) };
            if (sample.CallsPerFrame == 0) continue;

            m_Log.Message("[FirstPersonProbe] INFO: {}.", DescribeFirstPerson(sample));
        }

        m_Log.Message("[FirstPersonProbe] INFO: {}.",
            DescribeChud(m_RenderStore.GetHudHook(), m_RenderStore.GetFirstPersonSample(FirstPersonCaller::ViewPhase)));
    }

    auto RenderService::WaitForIdle(milliseconds timeout) -> bool
    {
        const auto deadline = steady_clock::now() + timeout;

        while (m_RenderStore.GetInFlight() > 0)
        {
            if (steady_clock::now() >= deadline) return false;
            std::this_thread::sleep_for(microseconds{ 100 });
        }

        return true;
    }

    auto RenderService::Shutdown() -> void
    {
        std::lock_guard lock{ m_ShutdownMutex };

        if (m_RenderStore.IsShutdownDone()) return;

        for (auto it = m_OnShutdown.rbegin(); it != m_OnShutdown.rend(); ++it)
        {
            try
            {
                (*it)();
            }
            catch (...)
            {
                this->ReportHandlerError("OnShutdown");
            }
        }

        m_RenderStore.Cleanup();
        m_RenderStore.SetShutdownDone();

        m_Log.Message("[RenderService] INFO: Shutdown complete.");
    }
}