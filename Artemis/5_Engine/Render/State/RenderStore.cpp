module;

#include <d3d11.h>
#include <wrl/client.h>

module Engine.Render.State;

namespace
{
    using Common::Engine::Type::k_NoHandle;

    using Engine::Render::Type::DrawPlace;
    using Engine::Render::Type::MaskPairKind;
    using Engine::Render::Type::MaskTextures;
    using Engine::Render::Type::SceneStatus;

    using Engine::Render::Type::k_MaskPairCount;

    auto MaskSlotOf(MaskPairKind kind) -> std::size_t
    {
        return (std::min)(static_cast<std::size_t>(kind), k_MaskPairCount - 1);
    }
}

namespace Engine::Render::State
{
    auto RenderStore::GetDevice() const -> ID3D11Device*
    {
        return m_Device.Get();
    }

    auto RenderStore::GetContext() const -> ID3D11DeviceContext*
    {
        return m_Context.Get();
    }

    auto RenderStore::GetWindow() const -> HWND
    {
        return m_Window;
    }

    auto RenderStore::GetRenderTarget() const -> ID3D11RenderTargetView*
    {
        return m_RenderTarget.Get();
    }

    auto RenderStore::GetDepthStencil() const -> ID3D11DepthStencilView*
    {
        return m_DepthStencil.Get();
    }

    auto RenderStore::GetSwapChain() const -> const void*
    {
        return m_SwapChain;
    }

    auto RenderStore::GetWidth() const -> std::uint32_t
    {
        return m_Width.load();
    }

    auto RenderStore::GetHeight() const -> std::uint32_t
    {
        return m_Height.load();
    }

    auto RenderStore::GetInFlight() const -> int
    {
        return m_InFlight.load(std::memory_order_acquire);
    }

    auto RenderStore::IsReady() const -> bool
    {
        return m_RenderTarget != nullptr;
    }

    auto RenderStore::IsResizing() const -> bool
    {
        return m_IsResizing.load();
    }

    auto RenderStore::IsShutdownRequested() const -> bool
    {
        return m_IsShutdownRequested.load(std::memory_order_acquire);
    }

    auto RenderStore::IsShutdownDone() const -> bool
    {
        return m_IsShutdownDone.load(std::memory_order_acquire);
    }

    auto RenderStore::SetDevice(ComPtr<ID3D11Device> device,
        ComPtr<ID3D11DeviceContext> context, HWND window,
        const void* swapChain) -> void
    {
        m_Device = std::move(device);
        m_Context = std::move(context);
        m_Window = window;
        m_SwapChain = swapChain;
    }

    auto RenderStore::SetTargets(ComPtr<ID3D11RenderTargetView> renderTarget,
        ComPtr<ID3D11DepthStencilView> depthStencil,
        std::uint32_t width, std::uint32_t height) -> void
    {
        m_RenderTarget = std::move(renderTarget);
        m_DepthStencil = std::move(depthStencil);

        m_Width.store(width);
        m_Height.store(height);
    }

    auto RenderStore::SetResizing(bool isResizing) -> void
    {
        m_IsResizing.store(isResizing);
    }

    auto RenderStore::SetShutdownDone() -> void
    {
        m_IsShutdownDone.store(true, std::memory_order_release);
    }

    auto RenderStore::RequestShutdown() -> void
    {
        m_IsShutdownRequested.store(true, std::memory_order_release);
    }

    auto RenderStore::IsGameHudHidden() const -> bool
    {
        return m_IsGameHudHidden.load(std::memory_order_relaxed);
    }

    auto RenderStore::SetGameHudHidden(bool isHidden) -> void
    {
        m_IsGameHudHidden.store(isHidden, std::memory_order_relaxed);
    }

    auto RenderStore::IsFirstPersonHidden() const -> bool
    {
        return m_IsFirstPersonHidden.load(std::memory_order_relaxed);
    }

    auto RenderStore::SetFirstPersonHidden(bool isHidden) -> void
    {
        m_IsFirstPersonHidden.store(isHidden, std::memory_order_relaxed);
    }

    auto RenderStore::SetFirstPersonHooksInstalled(bool isModelHooked, bool isUnitHooked, bool isDepthPassHooked, bool isOpaqueHooked) -> void
    {
        m_IsFirstPersonModelHooked.store(isModelHooked, std::memory_order_relaxed);
        m_IsFirstPersonUnitHooked.store(isUnitHooked, std::memory_order_relaxed);
        m_IsFirstPersonDepthPassHooked.store(isDepthPassHooked, std::memory_order_relaxed);
        m_IsOpaquePassHooked.store(isOpaqueHooked, std::memory_order_relaxed);
    }

    auto RenderStore::IsOpaquePassHooked() const -> bool
    {
        return m_IsOpaquePassHooked.load(std::memory_order_relaxed);
    }

    auto RenderStore::IsFirstPersonDepthPassHooked() const -> bool
    {
        return m_IsFirstPersonDepthPassHooked.load(std::memory_order_relaxed);
    }

    auto RenderStore::IsFirstPersonModelHooked() const -> bool
    {
        return m_IsFirstPersonModelHooked.load(std::memory_order_relaxed);
    }

    auto RenderStore::IsFirstPersonUnitHooked() const -> bool
    {
        return m_IsFirstPersonUnitHooked.load(std::memory_order_relaxed);
    }

    auto RenderStore::CountThirdPerson() -> void
    {
        m_ThirdPersonCalls.fetch_add(1, std::memory_order_relaxed);
    }

    auto RenderStore::WasThirdPersonLastFrame() const -> bool
    {
        return m_ThirdPersonLastFrame.load(std::memory_order_relaxed) != 0;
    }

    auto RenderStore::CountFirstPersonUnit(std::uint32_t objectHandle) -> void
    {
        m_FirstPersonUnitCalls.fetch_add(1, std::memory_order_relaxed);

        // The first object of the frame stays: a later answer finds the slot taken.
        std::uint32_t expected{ k_NoHandle };
        m_FirstPersonUnitObject.compare_exchange_strong(expected, objectHandle, std::memory_order_relaxed);
    }

    auto RenderStore::GetFirstPersonUnitObjectLastFrame() const -> std::uint32_t
    {
        return m_FirstPersonUnitObjectLastFrame.load(std::memory_order_relaxed);
    }

    auto RenderStore::WasFirstPersonUnitLastFrame() const -> bool
    {
        return m_FirstPersonUnitLastFrame.load(std::memory_order_relaxed) != 0;
    }

    auto RenderStore::GetPresentThread() const -> std::uint32_t
    {
        return m_PresentThread.load(std::memory_order_relaxed);
    }

    auto RenderStore::SetPresentThread(std::uint32_t threadId) -> void
    {
        m_PresentThread.store(threadId, std::memory_order_relaxed);
    }

    auto RenderStore::EndFrame() -> void
    {
        m_ThirdPersonLastFrame.store(m_ThirdPersonCalls.exchange(0, std::memory_order_relaxed), std::memory_order_relaxed);
        m_FirstPersonUnitLastFrame.store(m_FirstPersonUnitCalls.exchange(0, std::memory_order_relaxed), std::memory_order_relaxed);
        m_FirstPersonUnitObjectLastFrame.store(m_FirstPersonUnitObject.exchange(k_NoHandle, std::memory_order_relaxed), std::memory_order_relaxed);

        for (std::size_t i{ 0 }; i < k_MaskPairCount; ++i)
        {
            m_IsMaskCopiedLastFrame[i].store(m_IsMaskCopied[i].exchange(false, std::memory_order_acq_rel), std::memory_order_release);
            m_IsMaskTaken[i].store(false, std::memory_order_release);
        }

        m_MaskFrameDepth.Reset();
    }

    auto RenderStore::NextPresentFrame() -> void
    {
        m_SceneLastStatus.store(m_SceneStatus.exchange(SceneStatus::NoSceneCall, std::memory_order_relaxed), std::memory_order_relaxed);
        m_DrawPlaceLast.store(m_DrawPlace.exchange(DrawPlace::None, std::memory_order_relaxed), std::memory_order_relaxed);
        m_PresentIndex.fetch_add(1, std::memory_order_relaxed);
    }

    auto RenderStore::IsSceneDrawRequested() const -> bool
    {
        return m_IsSceneDrawRequested.load(std::memory_order_relaxed);
    }

    auto RenderStore::SetSceneDrawRequested(bool isRequested) -> void
    {
        m_IsSceneDrawRequested.store(isRequested, std::memory_order_relaxed);
    }

    auto RenderStore::TakeSceneCall() -> bool
    {
        const std::uint64_t token{ m_PresentIndex.load(std::memory_order_relaxed) + 1 };
        return m_SceneCallMark.exchange(token, std::memory_order_relaxed) != token;
    }

    auto RenderStore::SetSceneStatus(SceneStatus status) -> void
    {
        m_SceneStatus.store(status, std::memory_order_relaxed);
    }

    auto RenderStore::GetSceneStatus() const -> SceneStatus
    {
        return m_SceneLastStatus.load(std::memory_order_relaxed);
    }

    auto RenderStore::SetDrawPlace(DrawPlace place) -> void
    {
        m_DrawPlace.store(place, std::memory_order_relaxed);
    }

    auto RenderStore::GetDrawPlace() const -> DrawPlace
    {
        return m_DrawPlaceLast.load(std::memory_order_relaxed);
    }

    auto RenderStore::CountSceneFailure() -> void
    {
        m_SceneFailures.fetch_add(1, std::memory_order_relaxed);
    }

    auto RenderStore::GetSceneFailures() const -> std::uint32_t
    {
        return m_SceneFailures.load(std::memory_order_relaxed);
    }

    auto RenderStore::SetSceneColorSlider(float slider) -> void
    {
        m_SceneColorSlider.store(slider, std::memory_order_relaxed);
    }

    auto RenderStore::GetSceneColorSlider() const -> float
    {
        return m_SceneColorSlider.load(std::memory_order_relaxed);
    }

    auto RenderStore::SetSceneColorScale(float scale) -> void
    {
        m_SceneColorScale.store(scale, std::memory_order_relaxed);
    }

    auto RenderStore::GetSceneColorScale() const -> float
    {
        return m_SceneColorScale.load(std::memory_order_relaxed);
    }

    auto RenderStore::IsFirstPersonMaskRequested() const -> bool
    {
        return m_IsMaskRequested.load(std::memory_order_relaxed);
    }

    auto RenderStore::SetFirstPersonMaskRequested(bool isRequested) -> void
    {
        m_IsMaskRequested.store(isRequested, std::memory_order_relaxed);
    }

    auto RenderStore::TakeMaskCopy(MaskPairKind kind) -> bool
    {
        return !m_IsMaskTaken[MaskSlotOf(kind)].exchange(true, std::memory_order_acq_rel);
    }

    auto RenderStore::MarkMaskCopied(MaskPairKind kind) -> void
    {
        m_IsMaskCopied[MaskSlotOf(kind)].store(true, std::memory_order_release);
    }

    auto RenderStore::IsMaskCopied(MaskPairKind kind) const -> bool
    {
        return m_IsMaskCopied[MaskSlotOf(kind)].load(std::memory_order_acquire);
    }

    auto RenderStore::WasMaskCopiedLastFrame(MaskPairKind kind) const -> bool
    {
        return m_IsMaskCopiedLastFrame[MaskSlotOf(kind)].load(std::memory_order_acquire);
    }

    auto RenderStore::NoteMaskDesc(MaskPairKind kind, const D3D11_TEXTURE2D_DESC& desc) -> void
    {
        m_Masks[MaskSlotOf(kind)].Wanted = desc;
    }

    auto RenderStore::GetMaskWantedDesc(MaskPairKind kind) const -> std::optional<D3D11_TEXTURE2D_DESC>
    {
        return m_Masks[MaskSlotOf(kind)].Wanted;
    }

    auto RenderStore::SetMaskTextures(MaskPairKind kind, ComPtr<ID3D11Texture2D> pre, ComPtr<ID3D11Texture2D> post,
        ComPtr<ID3D11ShaderResourceView> preView, ComPtr<ID3D11ShaderResourceView> postView, const D3D11_TEXTURE2D_DESC& desc) -> void
    {
        MaskTextures& mask{ m_Masks[MaskSlotOf(kind)] };

        mask.Pre = std::move(pre);
        mask.Post = std::move(post);
        mask.PreView = std::move(preView);
        mask.PostView = std::move(postView);
        mask.Desc = desc;
        mask.IsAttempted = true;
    }

    auto RenderStore::IsMaskAttempted(MaskPairKind kind) const -> bool
    {
        return m_Masks[MaskSlotOf(kind)].IsAttempted;
    }

    auto RenderStore::AreMaskTexturesReady(MaskPairKind kind) const -> bool
    {
        const MaskTextures& mask{ m_Masks[MaskSlotOf(kind)] };
        return mask.Pre != nullptr && mask.Post != nullptr && mask.PreView != nullptr && mask.PostView != nullptr;
    }

    auto RenderStore::GetMaskDesc(MaskPairKind kind) const -> D3D11_TEXTURE2D_DESC
    {
        return m_Masks[MaskSlotOf(kind)].Desc;
    }

    auto RenderStore::GetMaskPre(MaskPairKind kind) const -> ID3D11Texture2D*
    {
        return m_Masks[MaskSlotOf(kind)].Pre.Get();
    }

    auto RenderStore::GetMaskPost(MaskPairKind kind) const -> ID3D11Texture2D*
    {
        return m_Masks[MaskSlotOf(kind)].Post.Get();
    }

    auto RenderStore::GetMaskPreView(MaskPairKind kind) const -> ID3D11ShaderResourceView*
    {
        return m_Masks[MaskSlotOf(kind)].PreView.Get();
    }

    auto RenderStore::GetMaskPostView(MaskPairKind kind) const -> ID3D11ShaderResourceView*
    {
        return m_Masks[MaskSlotOf(kind)].PostView.Get();
    }

    auto RenderStore::SetMaskSource(MaskPairKind kind, ID3D11Resource* source) -> void
    {
        m_Masks[MaskSlotOf(kind)].Source = source;
    }

    auto RenderStore::GetMaskSource(MaskPairKind kind) const -> ID3D11Resource*
    {
        return m_Masks[MaskSlotOf(kind)].Source.Get();
    }

    auto RenderStore::SetMaskFrameDepth(ID3D11Resource* resource) -> void
    {
        m_MaskFrameDepth = resource;
    }

    auto RenderStore::GetMaskFrameDepth() const -> ID3D11Resource*
    {
        return m_MaskFrameDepth.Get();
    }

    auto RenderStore::ReleaseMaskTextures() -> void
    {
        for (MaskTextures& mask : m_Masks) mask = MaskTextures{};

        m_MaskFrameDepth.Reset();
    }

    auto RenderStore::ReleaseMaskTextures(MaskPairKind kind) -> void
    {
        m_Masks[MaskSlotOf(kind)] = MaskTextures{};

        if (kind == MaskPairKind::DepthB) m_MaskFrameDepth.Reset();
    }

    auto RenderStore::ReleaseTargets() -> void
    {
        m_RenderTarget.Reset();
        m_DepthStencil.Reset();
        this->ReleaseMaskTextures();
    }

    auto RenderStore::Cleanup() -> void
    {
        this->ReleaseTargets();

        m_Context.Reset();
        m_Device.Reset();
        m_Window = nullptr;
        m_SwapChain = nullptr;

        m_Width.store(0);
        m_Height.store(0);
    }
}