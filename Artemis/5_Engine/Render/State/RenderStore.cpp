module;

#include <d3d11.h>
#include <wrl/client.h>

module Engine.Render.State;

namespace
{
    using Engine::Render::Type::BeforeHudStatus;
    using Engine::Render::Type::ColorPairNote;
    using Engine::Render::Type::DepthPairNote;
    using Engine::Render::Type::DrawPlace;
    using Engine::Render::Type::FirstPersonCaller;
    using Engine::Render::Type::FirstPersonSample;
    using Engine::Render::Type::HudHookInfo;
    using Engine::Render::Type::MaskPairKind;
    using Engine::Render::Type::MaskTextures;
    using Engine::Render::Type::MaskWindow;
    using Engine::Render::Type::MaskWindowReport;
    using Engine::Render::Type::SceneColorSample;
    using Engine::Render::Type::StencilProbeInfo;

    using Engine::Render::Type::k_ColorPairCount;
    using Engine::Render::Type::k_DepthPairCount;
    using Engine::Render::Type::k_FirstPersonCallerCount;
    using Engine::Render::Type::k_MaskPairCount;

    // States of the first person probe.
    constexpr std::uint8_t k_ProbeIdle{ 0 };
    constexpr std::uint8_t k_ProbeRequested{ 1 };
    constexpr std::uint8_t k_ProbeArmed{ 2 };

    auto MaskSlotOf(MaskPairKind kind) -> std::size_t
    {
        return (std::min)(static_cast<std::size_t>(kind), k_MaskPairCount - 1);
    }

    auto SlotOf(FirstPersonCaller caller) -> std::size_t
    {
        return (std::min)(static_cast<std::size_t>(caller), k_FirstPersonCallerCount - 1);
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

    auto RenderStore::SetFirstPersonHooksInstalled(bool isModelHooked, bool isUnitHooked, bool isDepthPassHooked, bool isForwardHooked) -> void
    {
        m_IsFirstPersonModelHooked.store(isModelHooked, std::memory_order_relaxed);
        m_IsFirstPersonUnitHooked.store(isUnitHooked, std::memory_order_relaxed);
        m_IsFirstPersonDepthPassHooked.store(isDepthPassHooked, std::memory_order_relaxed);
        m_IsFirstPersonForwardHooked.store(isForwardHooked, std::memory_order_relaxed);
    }

    auto RenderStore::IsFirstPersonForwardHooked() const -> bool
    {
        return m_IsFirstPersonForwardHooked.load(std::memory_order_relaxed);
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

    auto RenderStore::CountFirstPersonUnit() -> void
    {
        m_FirstPersonUnitCalls.fetch_add(1, std::memory_order_relaxed);
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

    auto RenderStore::SetHudHookInstalled(bool isInstalled) -> void
    {
        m_IsHudHookInstalled.store(isInstalled, std::memory_order_relaxed);
    }

    auto RenderStore::CountHudCall() -> void
    {
        m_HudCalls.fetch_add(1, std::memory_order_relaxed);
    }

    auto RenderStore::EndHudFrame() -> void
    {
        m_HudCallsLastFrame.store(m_HudCalls.exchange(0, std::memory_order_relaxed), std::memory_order_relaxed);
        m_ThirdPersonLastFrame.store(m_ThirdPersonCalls.exchange(0, std::memory_order_relaxed), std::memory_order_relaxed);
        m_FirstPersonUnitLastFrame.store(m_FirstPersonUnitCalls.exchange(0, std::memory_order_relaxed), std::memory_order_relaxed);
        std::array<bool, k_MaskPairCount> isCopied{};

        for (std::size_t i{ 0 }; i < k_MaskPairCount; ++i)
        {
            isCopied[i] = m_IsMaskCopied[i].exchange(false, std::memory_order_acq_rel);
            m_IsMaskCopiedLastFrame[i].store(isCopied[i], std::memory_order_release);
            m_IsMaskTaken[i].store(false, std::memory_order_release);
        }

        m_MaskFrameDepth.Reset();

        const std::uint32_t passCalls{ m_MaskPassCalls.exchange(0, std::memory_order_relaxed) };

        if (!this->IsFirstPersonMaskRequested())
        {
            // A window is only of frames with the mask requested, so a new request starts a new one.
            m_MaskWindow = MaskWindow{};
            m_IsMaskWindowPublished.store(false, std::memory_order_release);
        }
        else if (const auto report{ Engine::Render::Type::AdvanceMaskWindow(m_MaskWindow, isCopied, passCalls) })
        {
            m_MaskWindowFrames.store(report->Frames, std::memory_order_relaxed);

            for (std::size_t i{ 0 }; i < k_MaskPairCount; ++i) m_MaskWindowNoCopy[i].store(report->NoCopyFrames[i], std::memory_order_relaxed);

            m_MaskWindowMaxCalls.store(report->MaxPassCalls, std::memory_order_relaxed);
            m_IsMaskWindowPublished.store(true, std::memory_order_release);
        }

        for (std::size_t i{ 0 }; i < k_FirstPersonCallerCount; ++i)
        {
            m_FirstPersonCallsLastFrame[i].store(m_FirstPersonCalls[i].exchange(0, std::memory_order_relaxed), std::memory_order_relaxed);
        }
    }

    auto RenderStore::CountFirstPersonCall(FirstPersonCaller caller) -> void
    {
        m_FirstPersonCalls[SlotOf(caller)].fetch_add(1, std::memory_order_relaxed);
    }

    auto RenderStore::GetFirstPersonCalls(FirstPersonCaller caller) const -> std::uint32_t
    {
        return m_FirstPersonCallsLastFrame[SlotOf(caller)].load(std::memory_order_relaxed);
    }

    auto RenderStore::GetFirstPersonCallerCount() const -> std::uint32_t
    {
        std::uint32_t count{ 0 };

        for (const auto& calls : m_FirstPersonCallsLastFrame)
        {
            if (calls.load(std::memory_order_relaxed) != 0) ++count;
        }

        return count;
    }

    auto RenderStore::RequestFirstPersonProbe() -> void
    {
        std::uint8_t expected{ k_ProbeIdle };
        m_FirstPersonProbe.compare_exchange_strong(expected, k_ProbeRequested, std::memory_order_relaxed);
    }

    auto RenderStore::IsFirstPersonProbePending() const -> bool
    {
        return m_FirstPersonProbe.load(std::memory_order_relaxed) != k_ProbeIdle;
    }

    auto RenderStore::IsFirstPersonProbeArmed() const -> bool
    {
        return m_FirstPersonProbe.load(std::memory_order_relaxed) == k_ProbeArmed;
    }

    auto RenderStore::StepFirstPersonProbe() -> bool
    {
        const std::uint8_t state{ m_FirstPersonProbe.load(std::memory_order_relaxed) };

        if (state == k_ProbeArmed)
        {
            if (this->GetFirstPersonCallerCount() == 0) return false;

            // A captured depth resource without an attempt to make the depth pairs: the frame that made the copies was not measured yet.
            if (m_FirstPersonDepth != nullptr && !m_DepthPairs.GetSetup().IsAttempted) return false;

            // The same for the color pairs: the forward pass noted its target but the copies are not made yet.
            if (m_ColorDesc && !m_ColorPairs.GetSetup().IsAttempted) return false;

            m_FirstPersonProbe.store(k_ProbeIdle, std::memory_order_relaxed);
            this->ReleaseFirstPersonDepth();
            return true;
        }

        if (state == k_ProbeRequested)
        {
            {
                const std::lock_guard<std::mutex> lock{ m_FirstPersonSampleMutex };
                m_FirstPersonSamples = {};
            }

            this->ReleaseFirstPersonDepth();
            m_DepthPairs.Release();
            this->ReleaseColorPairs();

            {
                const std::lock_guard<std::mutex> lock{ m_StencilMutex };
                m_StencilProbe = {};
            }

            m_IsStencilAttempted.store(false, std::memory_order_relaxed);

            m_FirstPersonProbe.store(k_ProbeArmed, std::memory_order_relaxed);
        }

        return false;
    }

    auto RenderStore::CaptureFirstPersonDepth(ID3D11Resource* resource) -> void
    {
        if (resource == nullptr || m_FirstPersonDepth != nullptr) return;

        m_FirstPersonDepth = resource;
    }

    auto RenderStore::GetFirstPersonDepth() const -> ID3D11Resource*
    {
        return m_FirstPersonDepth.Get();
    }

    auto RenderStore::ReleaseFirstPersonDepth() -> void
    {
        m_FirstPersonDepth.Reset();
    }

    auto RenderStore::GetDepthPairs() -> CopyPairs<DepthPairNote, k_DepthPairCount>&
    {
        return m_DepthPairs;
    }

    auto RenderStore::GetDepthPairs() const -> const CopyPairs<DepthPairNote, k_DepthPairCount>&
    {
        return m_DepthPairs;
    }

    auto RenderStore::GetColorPairs() -> CopyPairs<ColorPairNote, k_ColorPairCount>&
    {
        return m_ColorPairs;
    }

    auto RenderStore::GetColorPairs() const -> const CopyPairs<ColorPairNote, k_ColorPairCount>&
    {
        return m_ColorPairs;
    }

    auto RenderStore::NoteColorDesc(const D3D11_TEXTURE2D_DESC& desc) -> void
    {
        if (!m_ColorDesc) m_ColorDesc = desc;
    }

    auto RenderStore::GetColorDesc() const -> std::optional<D3D11_TEXTURE2D_DESC>
    {
        return m_ColorDesc;
    }

    auto RenderStore::ReleaseColorPairs() -> void
    {
        m_ColorPairs.Release();
        m_ColorDesc.reset();
    }

    auto RenderStore::SetStencilProbe(const StencilProbeInfo& info) -> void
    {
        {
            const std::lock_guard<std::mutex> lock{ m_StencilMutex };
            m_StencilProbe = info;
        }

        m_IsStencilAttempted.store(info.IsAttempted, std::memory_order_relaxed);
    }

    auto RenderStore::GetStencilProbe() const -> StencilProbeInfo
    {
        const std::lock_guard<std::mutex> lock{ m_StencilMutex };
        return m_StencilProbe;
    }

    auto RenderStore::IsStencilProbeAttempted() const -> bool
    {
        return m_IsStencilAttempted.load(std::memory_order_relaxed);
    }

    auto RenderStore::SetFirstPersonDriveActive(bool isActive) -> void
    {
        m_IsFirstPersonDriveActive.store(isActive, std::memory_order_relaxed);
    }

    auto RenderStore::IsFirstPersonDriveActive() const -> bool
    {
        return m_IsFirstPersonDriveActive.load(std::memory_order_relaxed);
    }

    auto RenderStore::SetFirstPersonSample(const FirstPersonSample& sample) -> void
    {
        const std::lock_guard<std::mutex> lock{ m_FirstPersonSampleMutex };
        m_FirstPersonSamples[SlotOf(sample.Caller)] = sample;
    }

    auto RenderStore::GetFirstPersonSample(FirstPersonCaller caller) const -> FirstPersonSample
    {
        FirstPersonSample sample{};

        {
            const std::lock_guard<std::mutex> lock{ m_FirstPersonSampleMutex };
            sample = m_FirstPersonSamples[SlotOf(caller)];
        }

        sample.CallsPerFrame = this->GetFirstPersonCalls(caller);
        return sample;
    }

    auto RenderStore::CountHudFailure() -> void
    {
        m_HudFailures.fetch_add(1, std::memory_order_relaxed);
    }

    auto RenderStore::GetHudFailures() const -> std::uint32_t
    {
        return m_HudFailures.load(std::memory_order_relaxed);
    }

    auto RenderStore::NextPresentFrame() -> void
    {
        m_SceneLastStatus.store(m_SceneStatus.exchange(BeforeHudStatus::NoSceneCall, std::memory_order_relaxed), std::memory_order_relaxed);
        m_DrawPlaceLast.store(m_DrawPlace.exchange(DrawPlace::Present, std::memory_order_relaxed), std::memory_order_relaxed);
        m_BeforeHudLastStatus.store(m_BeforeHudStatus.exchange(BeforeHudStatus::NoHudDraw, std::memory_order_relaxed), std::memory_order_relaxed);
        m_PresentIndex.fetch_add(1, std::memory_order_relaxed);
    }

    auto RenderStore::MarkBeforeHud() -> void
    {
        m_BeforeHudMark.store(m_PresentIndex.load(std::memory_order_relaxed) + 1, std::memory_order_relaxed);
    }

    auto RenderStore::IsBeforeHudMarked() const -> bool
    {
        return m_BeforeHudMark.load(std::memory_order_relaxed) == m_PresentIndex.load(std::memory_order_relaxed) + 1;
    }

    auto RenderStore::SetBeforeHudStatus(BeforeHudStatus status) -> void
    {
        m_BeforeHudStatus.store(status, std::memory_order_relaxed);
    }

    auto RenderStore::GetBeforeHudStatus() const -> BeforeHudStatus
    {
        return m_BeforeHudLastStatus.load(std::memory_order_relaxed);
    }

    auto RenderStore::TakeSceneCall() -> bool
    {
        const std::uint64_t token{ m_PresentIndex.load(std::memory_order_relaxed) + 1 };
        return m_SceneCallMark.exchange(token, std::memory_order_relaxed) != token;
    }

    auto RenderStore::SetSceneStatus(BeforeHudStatus status) -> void
    {
        m_SceneStatus.store(status, std::memory_order_relaxed);
    }

    auto RenderStore::GetSceneStatus() const -> BeforeHudStatus
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

    auto RenderStore::SetSceneColorRequest(bool isByGameHdr, float slider) -> void
    {
        m_IsSceneColorByGameHdr.store(isByGameHdr, std::memory_order_relaxed);
        m_SceneColorSlider.store(slider, std::memory_order_relaxed);
    }

    auto RenderStore::IsSceneColorByGameHdr() const -> bool
    {
        return m_IsSceneColorByGameHdr.load(std::memory_order_relaxed);
    }

    auto RenderStore::GetSceneColorSlider() const -> float
    {
        return m_SceneColorSlider.load(std::memory_order_relaxed);
    }

    auto RenderStore::SetSceneColorSample(const SceneColorSample& sample) -> void
    {
        m_SceneColorHdrScale.store(sample.HdrScale, std::memory_order_relaxed);
        m_SceneColorViewScale.store(sample.ViewScale, std::memory_order_relaxed);
        m_SceneColorViewInverse.store(sample.ViewInverse, std::memory_order_relaxed);
        m_SceneColorScriptedScale.store(sample.ScriptedScale, std::memory_order_relaxed);
    }

    auto RenderStore::GetSceneColorSample() const -> SceneColorSample
    {
        return SceneColorSample{
            .HdrScale = m_SceneColorHdrScale.load(std::memory_order_relaxed),
            .ViewScale = m_SceneColorViewScale.load(std::memory_order_relaxed),
            .ViewInverse = m_SceneColorViewInverse.load(std::memory_order_relaxed),
            .ScriptedScale = m_SceneColorScriptedScale.load(std::memory_order_relaxed) };
    }

    auto RenderStore::CountSceneColorFailures(std::uint32_t count) -> void
    {
        m_SceneColorFailures.fetch_add(count, std::memory_order_relaxed);
    }

    auto RenderStore::GetSceneColorFailures() const -> std::uint32_t
    {
        return m_SceneColorFailures.load(std::memory_order_relaxed);
    }

    auto RenderStore::SetSceneColorScale(float scale) -> void
    {
        m_SceneColorScale.store(scale, std::memory_order_relaxed);
    }

    auto RenderStore::GetSceneColorScale() const -> float
    {
        return m_SceneColorScale.load(std::memory_order_relaxed);
    }

    auto RenderStore::SetHudSample(const HudHookInfo& sample) -> void
    {
        const std::lock_guard<std::mutex> lock{ m_HudSampleMutex };
        m_HudSample = sample;
    }

    auto RenderStore::GetHudHook() const -> HudHookInfo
    {
        HudHookInfo info{};

        {
            const std::lock_guard<std::mutex> lock{ m_HudSampleMutex };
            info = m_HudSample;
        }

        info.IsHooked = m_IsHudHookInstalled.load(std::memory_order_relaxed);
        info.CallsPerFrame = m_HudCallsLastFrame.load(std::memory_order_relaxed);
        return info;
    }

    auto RenderStore::IsFirstPersonMaskRequested() const -> bool
    {
        return m_IsMaskRequested.load(std::memory_order_relaxed);
    }

    auto RenderStore::SetFirstPersonMaskRequested(bool isRequested) -> void
    {
        m_IsMaskRequested.store(isRequested, std::memory_order_relaxed);
    }

    auto RenderStore::IsOverlayInSceneRequested() const -> bool
    {
        return m_IsOverlayInSceneRequested.load(std::memory_order_relaxed);
    }

    auto RenderStore::SetOverlayInSceneRequested(bool isRequested) -> void
    {
        m_IsOverlayInSceneRequested.store(isRequested, std::memory_order_relaxed);
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

    auto RenderStore::CountMaskPassCall() -> void
    {
        m_MaskPassCalls.fetch_add(1, std::memory_order_relaxed);
    }

    auto RenderStore::GetMaskWindowReport() const -> std::optional<MaskWindowReport>
    {
        if (!m_IsMaskWindowPublished.load(std::memory_order_acquire)) return std::nullopt;

        MaskWindowReport report{ .Frames = m_MaskWindowFrames.load(std::memory_order_relaxed),
            .MaxPassCalls = m_MaskWindowMaxCalls.load(std::memory_order_relaxed) };

        for (std::size_t i{ 0 }; i < k_MaskPairCount; ++i) report.NoCopyFrames[i] = m_MaskWindowNoCopy[i].load(std::memory_order_relaxed);

        return report;
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
        this->ReleaseFirstPersonDepth();
        m_DepthPairs.Release();
        this->ReleaseColorPairs();
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