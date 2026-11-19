module;

#include <d3d11.h>
#include <wrl/client.h>

module Engine.Render.State;

namespace
{
    using Engine::Render::Type::BeforeHudStatus;
    using Engine::Render::Type::DepthPairNote;
    using Engine::Render::Type::DepthPairSetup;
    using Engine::Render::Type::FirstPersonCaller;
    using Engine::Render::Type::FirstPersonSample;
    using Engine::Render::Type::HudHookInfo;
    using Engine::Render::Type::MaskWindow;
    using Engine::Render::Type::MaskWindowReport;
    using Engine::Render::Type::StencilProbeInfo;

    using Engine::Render::Type::k_DepthCopyCount;
    using Engine::Render::Type::k_DepthPairCount;
    using Engine::Render::Type::k_FirstPersonCallerCount;

    // States of the first person probe.
    constexpr std::uint8_t k_ProbeIdle{ 0 };
    constexpr std::uint8_t k_ProbeRequested{ 1 };
    constexpr std::uint8_t k_ProbeArmed{ 2 };

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

    auto RenderStore::SetFirstPersonHooksInstalled(bool isModelHooked, bool isUnitHooked, bool isDepthPassHooked) -> void
    {
        m_IsFirstPersonModelHooked.store(isModelHooked, std::memory_order_relaxed);
        m_IsFirstPersonUnitHooked.store(isUnitHooked, std::memory_order_relaxed);
        m_IsFirstPersonDepthPassHooked.store(isDepthPassHooked, std::memory_order_relaxed);
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
        const bool isCopied{ m_IsMaskCopied.exchange(false, std::memory_order_acq_rel) };
        m_IsMaskCopiedLastFrame.store(isCopied, std::memory_order_release);
        m_IsMaskCallTaken.store(false, std::memory_order_release);

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
            m_MaskWindowNoCopy.store(report->NoCopyFrames, std::memory_order_relaxed);
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
            if (m_FirstPersonDepth != nullptr && !m_DepthPairSetup.IsAttempted) return false;

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
            this->ReleaseDepthPairs();

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

    auto RenderStore::SetDepthPairs(std::array<ComPtr<ID3D11Texture2D>, k_DepthCopyCount> copies, ComPtr<ID3D11Texture2D> staging,
        const D3D11_TEXTURE2D_DESC& desc) -> void
    {
        m_DepthCopies = std::move(copies);
        m_DepthStaging = std::move(staging);
        m_DepthPairDesc = desc;
    }

    auto RenderStore::AreDepthPairsReady() const -> bool
    {
        return m_DepthStaging != nullptr;
    }

    auto RenderStore::SetDepthPairSetup(const DepthPairSetup& setup) -> void
    {
        m_DepthPairSetup = setup;
    }

    auto RenderStore::GetDepthPairSetup() const -> DepthPairSetup
    {
        return m_DepthPairSetup;
    }

    auto RenderStore::GetDepthPairDesc() const -> D3D11_TEXTURE2D_DESC
    {
        return m_DepthPairDesc;
    }

    auto RenderStore::GetDepthCopy(std::size_t index) const -> ID3D11Texture2D*
    {
        return index < m_DepthCopies.size() ? m_DepthCopies[index].Get() : nullptr;
    }

    auto RenderStore::GetDepthStaging() const -> ID3D11Texture2D*
    {
        return m_DepthStaging.Get();
    }

    auto RenderStore::NoteDepthPair(const DepthPairNote& note, ID3D11Resource* source) -> std::optional<std::size_t>
    {
        if (m_DepthNoteCount >= k_DepthPairCount) return std::nullopt;

        const std::size_t index{ m_DepthNoteCount++ };
        m_DepthNotes[index] = note;
        m_DepthSources[index] = source;
        return index;
    }

    auto RenderStore::GetDepthPairNote(std::size_t index) const -> DepthPairNote
    {
        return index < m_DepthNoteCount ? m_DepthNotes[index] : DepthPairNote{};
    }

    auto RenderStore::GetDepthPairNoteCount() const -> std::uint32_t
    {
        return m_DepthNoteCount;
    }

    auto RenderStore::GetDepthPairSource(std::size_t index) const -> ID3D11Resource*
    {
        return index < m_DepthNoteCount ? m_DepthSources[index].Get() : nullptr;
    }

    auto RenderStore::ReleaseDepthPairs() -> void
    {
        for (auto& copy : m_DepthCopies) copy.Reset();
        for (auto& source : m_DepthSources) source.Reset();

        m_DepthStaging.Reset();
        m_DepthPairDesc = {};
        m_DepthPairSetup = {};
        m_DepthNotes = {};
        m_DepthNoteCount = 0;
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

    auto RenderStore::TakeMaskCopy() -> bool
    {
        return !m_IsMaskCallTaken.exchange(true, std::memory_order_acq_rel);
    }

    auto RenderStore::MarkMaskCopied() -> void
    {
        m_IsMaskCopied.store(true, std::memory_order_release);
    }

    auto RenderStore::IsMaskCopied() const -> bool
    {
        return m_IsMaskCopied.load(std::memory_order_acquire);
    }

    auto RenderStore::WasMaskCopiedLastFrame() const -> bool
    {
        return m_IsMaskCopiedLastFrame.load(std::memory_order_acquire);
    }

    auto RenderStore::CountMaskPassCall() -> void
    {
        m_MaskPassCalls.fetch_add(1, std::memory_order_relaxed);
    }

    auto RenderStore::GetMaskWindowReport() const -> std::optional<MaskWindowReport>
    {
        if (!m_IsMaskWindowPublished.load(std::memory_order_acquire)) return std::nullopt;

        return MaskWindowReport{ m_MaskWindowFrames.load(std::memory_order_relaxed), m_MaskWindowNoCopy.load(std::memory_order_relaxed),
            m_MaskWindowMaxCalls.load(std::memory_order_relaxed) };
    }

    auto RenderStore::NoteMaskDepthDesc(const D3D11_TEXTURE2D_DESC& desc) -> void
    {
        m_MaskDepthDesc = desc;
    }

    auto RenderStore::GetMaskDepthDesc() const -> std::optional<D3D11_TEXTURE2D_DESC>
    {
        return m_MaskDepthDesc;
    }

    auto RenderStore::SetMaskTextures(ComPtr<ID3D11Texture2D> pre, ComPtr<ID3D11Texture2D> post, ComPtr<ID3D11ShaderResourceView> preView,
        ComPtr<ID3D11ShaderResourceView> postView, const D3D11_TEXTURE2D_DESC& desc) -> void
    {
        m_MaskPre = std::move(pre);
        m_MaskPost = std::move(post);
        m_MaskPreView = std::move(preView);
        m_MaskPostView = std::move(postView);
        m_MaskDesc = desc;
        m_IsMaskAttempted = true;
    }

    auto RenderStore::IsMaskAttempted() const -> bool
    {
        return m_IsMaskAttempted;
    }

    auto RenderStore::AreMaskTexturesReady() const -> bool
    {
        return m_MaskPre != nullptr && m_MaskPost != nullptr && m_MaskPreView != nullptr && m_MaskPostView != nullptr;
    }

    auto RenderStore::GetMaskDesc() const -> D3D11_TEXTURE2D_DESC
    {
        return m_MaskDesc;
    }

    auto RenderStore::GetMaskPre() const -> ID3D11Texture2D*
    {
        return m_MaskPre.Get();
    }

    auto RenderStore::GetMaskPost() const -> ID3D11Texture2D*
    {
        return m_MaskPost.Get();
    }

    auto RenderStore::GetMaskPreView() const -> ID3D11ShaderResourceView*
    {
        return m_MaskPreView.Get();
    }

    auto RenderStore::GetMaskPostView() const -> ID3D11ShaderResourceView*
    {
        return m_MaskPostView.Get();
    }

    auto RenderStore::SetMaskSource(ID3D11Resource* source) -> void
    {
        m_MaskSource = source;
    }

    auto RenderStore::GetMaskSource() const -> ID3D11Resource*
    {
        return m_MaskSource.Get();
    }

    auto RenderStore::ReleaseMaskTextures() -> void
    {
        m_MaskPre.Reset();
        m_MaskPost.Reset();
        m_MaskPreView.Reset();
        m_MaskPostView.Reset();
        m_MaskSource.Reset();
        m_MaskDesc = {};
        m_MaskDepthDesc.reset();
        m_IsMaskAttempted = false;
    }

    auto RenderStore::ReleaseTargets() -> void
    {
        m_RenderTarget.Reset();
        m_DepthStencil.Reset();
        this->ReleaseFirstPersonDepth();
        this->ReleaseDepthPairs();
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