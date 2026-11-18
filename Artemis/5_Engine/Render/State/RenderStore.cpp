module;

#include <d3d11.h>
#include <wrl/client.h>

module Engine.Render.State;

namespace
{
    using Engine::Render::Type::BeforeHudStatus;
    using Engine::Render::Type::FirstPersonCaller;
    using Engine::Render::Type::FirstPersonSample;
    using Engine::Render::Type::HudHookInfo;

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

    auto RenderStore::SetFirstPersonHooksInstalled(bool isModelHooked, bool isUnitHooked) -> void
    {
        m_IsFirstPersonModelHooked.store(isModelHooked, std::memory_order_relaxed);
        m_IsFirstPersonUnitHooked.store(isUnitHooked, std::memory_order_relaxed);
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

            m_FirstPersonProbe.store(k_ProbeIdle, std::memory_order_relaxed);
            return true;
        }

        if (state == k_ProbeRequested)
        {
            {
                const std::lock_guard<std::mutex> lock{ m_FirstPersonSampleMutex };
                m_FirstPersonSamples = {};
            }

            m_FirstPersonProbe.store(k_ProbeArmed, std::memory_order_relaxed);
        }

        return false;
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

    auto RenderStore::ReleaseTargets() -> void
    {
        m_RenderTarget.Reset();
        m_DepthStencil.Reset();
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