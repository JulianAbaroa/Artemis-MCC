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
    using Engine::Render::Type::DrawPlace;
    using Engine::Render::Type::FormatName;
    using Engine::Render::Type::MaskContext;
    using Engine::Render::Type::MaskPairKind;
    using Engine::Render::Type::MaskPairView;
    using Engine::Render::Type::MaskPlan;
    using Engine::Render::Type::MaskSource;
    using Engine::Render::Type::SceneStatus;
    using Engine::Render::Type::SceneTargetSample;

    using Engine::Render::Type::ChooseZeroMaskSource;

    using Engine::Render::Type::k_FirstPersonMaskPass;
    using Engine::Render::Type::k_FirstPersonZeroPass;
    using Engine::Render::Type::k_MaskPairCount;

    using std::chrono::steady_clock;
    using std::chrono::microseconds;

    // return: Why the overlay cannot draw at the scene point with the target bound in slot 0, or nothing. The target is the HDR target of the game
    // (one sample, the size of the back buffer).
    auto FindTargetBlocker(const SceneTargetSample& sample, bool isReady, std::uint32_t width, std::uint32_t height)
        -> std::optional<SceneStatus>
    {
        if (!sample.IsSameThread) return SceneStatus::OtherThread;
        if (!isReady) return SceneStatus::NotReady;
        if (!sample.HasTarget || width == 0 || height == 0) return SceneStatus::TargetMismatch;
        if (!sample.IsSameDevice) return SceneStatus::OtherDevice;
        if (sample.Width != width || sample.Height != height) return SceneStatus::TargetMismatch;
        if (sample.SampleCount != 1) return SceneStatus::Multisampled;

        const bool isWholeTarget{ sample.HasViewport && sample.ViewportX == 0.0f && sample.ViewportY == 0.0f &&
            sample.ViewportWidth == static_cast<float>(width) && sample.ViewportHeight == static_cast<float>(height) };
        if (!isWholeTarget) return SceneStatus::SplitViewport;

        return std::nullopt;
    }

    // return: The resource of the depth stencil view bound now, null when none.
    auto BoundDepthResource(ID3D11DeviceContext* context) -> ComPtr<ID3D11Resource>
    {
        ComPtr<ID3D11DepthStencilView> view{};
        context->OMGetRenderTargets(0, nullptr, view.GetAddressOf());

        ComPtr<ID3D11Resource> resource{};
        if (view != nullptr) view->GetResource(resource.GetAddressOf());
        return resource;
    }

    // Whether a copy made from a can take the content of a texture of description b.
    auto IsSameTextureDesc(const D3D11_TEXTURE2D_DESC& a, const D3D11_TEXTURE2D_DESC& b) -> bool
    {
        return a.Width == b.Width && a.Height == b.Height && a.MipLevels == b.MipLevels && a.ArraySize == b.ArraySize &&
            a.Format == b.Format && a.SampleDesc.Count == b.SampleDesc.Count && a.SampleDesc.Quality == b.SampleDesc.Quality;
    }

    // return: How the log names a pair of the overlay mask.
    auto MaskPairName(MaskPairKind kind) -> const char*
    {
        switch (kind)
        {
        case MaskPairKind::DepthB: return "depth 0xB";
        case MaskPairKind::DepthZero: return "depth 0";
        }

        return "?";
    }

    // The textures of a pair of the overlay mask as the creation left them.
    struct MaskCreation
    {
        std::array<ComPtr<ID3D11Texture2D>, 2> Textures{};
        std::array<ComPtr<ID3D11ShaderResourceView>, 2> Views{};
        HRESULT Result{ S_OK };
    };

    // Makes the before and after textures of a pair, DEFAULT with only the shader resource bind, and a view of each with the format.
    auto CreateMaskTextures(ID3D11Device* device, const D3D11_TEXTURE2D_DESC& wanted, DXGI_FORMAT viewFormat) -> MaskCreation
    {
        D3D11_TEXTURE2D_DESC copyDesc{ wanted };
        copyDesc.Usage = D3D11_USAGE_DEFAULT;
        copyDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        copyDesc.CPUAccessFlags = 0;
        copyDesc.MiscFlags = 0;

        D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc{};
        viewDesc.Format = viewFormat;
        viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        viewDesc.Texture2D.MostDetailedMip = 0;
        viewDesc.Texture2D.MipLevels = 1;

        MaskCreation creation{ .Result = device != nullptr ? S_OK : E_POINTER };

        for (std::size_t i{ 0 }; i < creation.Textures.size() && SUCCEEDED(creation.Result); ++i)
        {
            creation.Result = device->CreateTexture2D(&copyDesc, nullptr, creation.Textures[i].GetAddressOf());

            if (SUCCEEDED(creation.Result))
            {
                creation.Result = device->CreateShaderResourceView(creation.Textures[i].Get(), &viewDesc, creation.Views[i].GetAddressOf());
            }
        }

        return creation;
    }

    // Reads the texture, the view and the device of a render target. The caller owns the view.
    auto MeasureColorTarget(ID3D11RenderTargetView* target, ID3D11Device* storeDevice) -> ColorTargetInfo
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

        info.SampleCount = textureDesc.SampleDesc.Count;

        ComPtr<ID3D11Device> targetDevice{};
        target->GetDevice(targetDevice.GetAddressOf());
        info.IsSameDevice = targetDevice != nullptr && targetDevice.Get() == storeDevice;
        return info;
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

    auto RenderService::OnScene(SceneHandler handler) -> void
    {
        m_OnScene.push_back(std::move(handler));
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

        this->PrepareMask();

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

    auto RenderService::MakeMask(bool isInFrame) const -> MaskContext
    {
        MaskContext mask{};

        if (!m_RenderStore.IsFirstPersonMaskRequested()) return mask;

        for (std::size_t i{ 0 }; i < k_MaskPairCount; ++i)
        {
            const MaskPairKind kind{ static_cast<MaskPairKind>(i) };

            if (!m_RenderStore.AreMaskTexturesReady(kind)) continue;

            const D3D11_TEXTURE2D_DESC desc{ m_RenderStore.GetMaskDesc(kind) };
            MaskPairView& pair{ mask.Pairs[i] };

            pair.IsValid = isInFrame ? m_RenderStore.IsMaskCopied(kind) : m_RenderStore.WasMaskCopiedLastFrame(kind);
            pair.Pre = m_RenderStore.GetMaskPreView(kind);
            pair.Post = m_RenderStore.GetMaskPostView(kind);
            pair.Width = desc.Width;
            pair.Height = desc.Height;
        }

        return mask;
    }

    auto RenderService::MakeContext(bool isInFrame) const -> FrameContext
    {
        FrameContext frame{};
        frame.Device = m_RenderStore.GetDevice();
        frame.Context = m_RenderStore.GetContext();
        frame.RenderTarget = m_RenderStore.GetRenderTarget();
        frame.DepthStencil = m_RenderStore.GetDepthStencil();
        frame.Window = m_RenderStore.GetWindow();
        frame.Width = m_RenderStore.GetWidth();
        frame.Height = m_RenderStore.GetHeight();
        frame.Mask = this->MakeMask(isInFrame);
        return frame;
    }

    auto RenderService::Raise(const std::vector<FrameHandler>& handlers,
        const char* stage) -> void
    {
        const FrameContext frame = this->MakeContext(false);

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

    auto RenderService::FindSceneBlocker(const SceneTargetSample& sample, bool isReady, std::uint32_t width, std::uint32_t height)
        -> std::optional<SceneStatus>
    {
        return FindTargetBlocker(sample, isReady, width, height);
    }

    auto RenderService::RunSceneHandlers(const FrameContext& frame, ID3D11DeviceContext* context) -> SceneStatus
    {
        bool isDrawn{ false };
        bool isFailed{ false };

        {
            GpuStateGuard guard{ context };

            for (const auto& handler : m_OnScene)
            {
                try
                {
                    if (handler(frame)) isDrawn = true;
                }
                catch (...)
                {
                    isFailed = true;
                    this->ReportHandlerError("OnScene");
                }
            }
        }

        return isFailed ? SceneStatus::Failed : isDrawn ? SceneStatus::Drawn : SceneStatus::Declined;
    }

    auto RenderService::OnSceneDraw() -> void
    {
        try
        {
            this->HandleSceneDraw();
        }
        catch (...)
        {
            m_RenderStore.CountSceneFailure();
        }
    }

    auto RenderService::ComputeSceneColorScale(float slider) -> float
    {
        return std::isfinite(slider) && slider > 0.0f ? slider : 1.0f;
    }

    auto RenderService::HandleSceneDraw() -> void
    {
        // note: Without a request (the Viewer is inactive) nothing is measured, no context is made and no state is saved.
        if (!m_RenderStore.IsSceneDrawRequested())
        {
            m_RenderStore.SetSceneStatus(SceneStatus::NotRequested);
            return;
        }

        const SceneTargetSample sample{ this->MeasureBoundTarget() };

        const bool isReady{ m_RenderStore.IsReady() && !m_RenderStore.IsResizing() && !m_RenderStore.IsShutdownRequested() };

        if (const auto blocker{ RenderService::FindSceneBlocker(sample, isReady, m_RenderStore.GetWidth(), m_RenderStore.GetHeight()) })
        {
            m_RenderStore.SetSceneStatus(*blocker);
            return;
        }

        // Only a call that passes the blocker (so on the Present thread) takes the point of the frame: a blocked call of another view
        // does not use it up, and a later call of the main view can still draw.
        if (!m_RenderStore.TakeSceneCall()) return;

        ID3D11DeviceContext* context{ m_RenderStore.GetContext() };

        ComPtr<ID3D11RenderTargetView> target{};
        if (context != nullptr) context->OMGetRenderTargets(1, target.GetAddressOf(), nullptr);

        if (context == nullptr || target == nullptr)
        {
            m_RenderStore.SetSceneStatus(SceneStatus::NotReady);
            return;
        }

        FrameContext frame{ this->MakeContext(true) };
        frame.RenderTarget = target.Get();
        frame.Width = sample.Width;
        frame.Height = sample.Height;

        // note: Only the scene point scales the color.
        frame.ColorScale = RenderService::ComputeSceneColorScale(m_RenderStore.GetSceneColorSlider());
        m_RenderStore.SetSceneColorScale(frame.ColorScale);

        const SceneStatus status{ this->RunSceneHandlers(frame, context) };
        m_RenderStore.SetSceneStatus(status);

        if (status == SceneStatus::Declined) return;

        m_RenderStore.SetDrawPlace(DrawPlace::Scene);

        if (!m_IsSceneLogged)
        {
            const char* format{ FormatName(sample.Format) };
            m_IsSceneLogged = true;
            m_Log.Message("[RenderService] INFO: The overlay draws in the scene, target format {} ({}), {}x{}.", format != nullptr ? format : "unknown", sample.Format, sample.Width, sample.Height);
        }
    }

    auto RenderService::MeasureBoundTarget() -> SceneTargetSample
    {
        SceneTargetSample sample{};

        const std::uint32_t presentThread{ m_RenderStore.GetPresentThread() };
        sample.IsSameThread = presentThread != 0 && presentThread == GetCurrentThreadId();

        if (!sample.IsSameThread || m_RenderStore.IsResizing() || !m_RenderStore.IsReady())
        {
            return sample;
        }

        ID3D11DeviceContext* context{ m_RenderStore.GetContext() };

        if (context == nullptr)
        {
            return sample;
        }

        ComPtr<ID3D11RenderTargetView> target{};
        context->OMGetRenderTargets(1, target.GetAddressOf(), nullptr);

        const ColorTargetInfo color{ MeasureColorTarget(target.Get(), m_RenderStore.GetDevice()) };

        if (color.HasTarget)
        {
            sample.HasTarget = true;
            sample.Width = color.Width;
            sample.Height = color.Height;
            sample.SampleCount = color.SampleCount;
            sample.Format = color.ViewFormat != DXGI_FORMAT_UNKNOWN ? color.ViewFormat : color.ResourceFormat;
            sample.IsSameDevice = color.IsSameDevice;
        }

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

        return sample;
    }

    auto RenderService::GetPresentContext() -> ID3D11DeviceContext*
    {
        if (m_RenderStore.IsResizing() || !m_RenderStore.IsReady()) return nullptr;

        const std::uint32_t presentThread{ m_RenderStore.GetPresentThread() };
        if (presentThread == 0 || presentThread != GetCurrentThreadId()) return nullptr;

        return m_RenderStore.GetContext();
    }

    auto RenderService::PlanMask(bool isRequested, const std::optional<D3D11_TEXTURE2D_DESC>& wanted, bool isAttempted,
        const D3D11_TEXTURE2D_DESC& attempted) -> MaskPlan
    {
        if (!isRequested) return isAttempted ? MaskPlan::Release : MaskPlan::Keep;
        if (!wanted) return MaskPlan::Keep;
        if (!isAttempted || !IsSameTextureDesc(*wanted, attempted)) return MaskPlan::Create;

        return MaskPlan::Keep;
    }

    auto RenderService::ScaleToMask(std::uint32_t pixel, std::uint32_t targetSize, std::uint32_t maskSize) -> std::uint32_t
    {
        if (targetSize == 0 || maskSize == 0) return 0;

        const std::uint64_t texel{ static_cast<std::uint64_t>(pixel) * maskSize / targetSize };
        return static_cast<std::uint32_t>((std::min)(texel, static_cast<std::uint64_t>(maskSize - 1)));
    }

    auto RenderService::BeginMaskPairCopy(MaskPairKind kind, ID3D11DeviceContext* context, ID3D11Resource* resource, bool canNote) -> bool
    {
        ComPtr<ID3D11Texture2D> texture{};
        if (resource != nullptr) resource->QueryInterface(IID_PPV_ARGS(texture.GetAddressOf()));
        if (texture == nullptr) return false;

        D3D11_TEXTURE2D_DESC desc{};
        texture->GetDesc(&desc);

        if (!m_RenderStore.AreMaskTexturesReady(kind) || !IsSameTextureDesc(desc, m_RenderStore.GetMaskDesc(kind)))
        {
            if (canNote) m_RenderStore.NoteMaskDesc(kind, desc);
            return false;
        }

        // The copy of the frame is taken only once a resource of the expected description is bound, so a call without one (another view) leaves
        // it to the next call of the same frame.
        if (!m_RenderStore.TakeMaskCopy(kind)) return false;

        m_RenderStore.SetMaskSource(kind, resource);
        context->CopyResource(m_RenderStore.GetMaskPre(kind), resource);
        return true;
    }

    auto RenderService::BeginMaskCopy(std::int32_t pass) -> std::optional<MaskPairKind>
    {
        if (!m_RenderStore.IsFirstPersonMaskRequested()) return std::nullopt;

        MaskPairKind kind{ MaskPairKind::DepthB };

        if (pass == k_FirstPersonZeroPass) kind = MaskPairKind::DepthZero;
        else if (pass != k_FirstPersonMaskPass) return std::nullopt;

        ID3D11DeviceContext* context{ this->GetPresentContext() };
        if (context == nullptr) return std::nullopt;

        ComPtr<ID3D11Resource> resource{ BoundDepthResource(context) };

        // The pass 0 call can run with no depth stencil bound (the game binds its own inside): the one of the pass 0xB call of the frame is the
        // same resource for the whole view.
        if (kind == MaskPairKind::DepthZero)
        {
            ComPtr<ID3D11Resource> saved{ m_RenderStore.GetMaskFrameDepth() };

            switch (ChooseZeroMaskSource(resource != nullptr, saved != nullptr))
            {
            case MaskSource::Bound:
                break;

            case MaskSource::Saved:
                resource = std::move(saved);
                break;

            case MaskSource::None:
                return std::nullopt;
            }
        }

        // The pass 0 pair is made from the description of the pass 0xB pair, so it never notes one.
        if (!this->BeginMaskPairCopy(kind, context, resource.Get(), kind == MaskPairKind::DepthB)) return std::nullopt;

        if (kind == MaskPairKind::DepthB) m_RenderStore.SetMaskFrameDepth(resource.Get());

        return kind;
    }

    auto RenderService::EndMaskCopy(MaskPairKind kind) -> void
    {
        ID3D11DeviceContext* context{ this->GetPresentContext() };
        ID3D11Resource* source{ m_RenderStore.GetMaskSource(kind) };
        ID3D11Texture2D* post{ m_RenderStore.GetMaskPost(kind) };

        if (context != nullptr && source != nullptr && post != nullptr)
        {
            context->CopyResource(post, source);
            m_RenderStore.MarkMaskCopied(kind);
        }

        m_RenderStore.SetMaskSource(kind, nullptr);
    }

    auto RenderService::PrepareMask() -> void
    {
        if (m_RenderStore.IsResizing() || !m_RenderStore.IsReady()) return;

        for (std::size_t i{ 0 }; i < k_MaskPairCount; ++i) this->PrepareMaskPair(static_cast<MaskPairKind>(i));
    }

    auto RenderService::PrepareMaskPair(MaskPairKind kind) -> void
    {
        const std::optional<D3D11_TEXTURE2D_DESC> wanted{ m_RenderStore.GetMaskWantedDesc(kind == MaskPairKind::DepthZero ? MaskPairKind::DepthB : kind) };

        switch (RenderService::PlanMask(m_RenderStore.IsFirstPersonMaskRequested(), wanted, m_RenderStore.IsMaskAttempted(kind),
            m_RenderStore.GetMaskDesc(kind)))
        {
        case MaskPlan::Keep:
            return;

        case MaskPlan::Release:
            m_RenderStore.ReleaseMaskTextures(kind);
            return;

        case MaskPlan::Create:
            // A pair copied in the last frame keeps its textures: another call of the frame with a different target (another view or phase)
            // notes its description too, and recreating from it would flip the textures every frame and never copy.
            if (m_RenderStore.WasMaskCopiedLastFrame(kind)) return;
            break;
        }

        // The depth is read as R32_FLOAT_X8X24 without multisampling: another texture is attempted once and logged.
        if (wanted->Format != DXGI_FORMAT_R32G8X24_TYPELESS || wanted->SampleDesc.Count != 1)
        {
            m_RenderStore.SetMaskTextures(kind, {}, {}, {}, {}, *wanted);
            m_Log.Message("[RenderService] WARNING: The {} texture of the first person mask cannot be a source (format {}, samples {}).",
                MaskPairName(kind), static_cast<std::uint32_t>(wanted->Format), wanted->SampleDesc.Count);
            return;
        }

        MaskCreation creation{ CreateMaskTextures(m_RenderStore.GetDevice(), *wanted, DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS) };

        if (FAILED(creation.Result))
        {
            m_RenderStore.SetMaskTextures(kind, {}, {}, {}, {}, *wanted);
            m_Log.Message("[RenderService] ERROR: Failed to create the first person mask textures ({}), hresult {:#010X}.", MaskPairName(kind),
                static_cast<std::uint32_t>(creation.Result));
            return;
        }

        m_RenderStore.SetMaskTextures(kind, std::move(creation.Textures[0]), std::move(creation.Textures[1]), std::move(creation.Views[0]),
            std::move(creation.Views[1]), *wanted);
        m_Log.Message("[RenderService] INFO: First person mask textures made ({}), {}x{}.", MaskPairName(kind), wanted->Width, wanted->Height);
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