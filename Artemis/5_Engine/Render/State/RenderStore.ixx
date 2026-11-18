module;

#include <d3d11.h>
#include <wrl/client.h>

export module Engine.Render.State;

import Engine.Render.Type;
import std;

namespace
{
    using Engine::Render::Type::BeforeHudStatus;
    using Engine::Render::Type::HudHookInfo;
}

export namespace Engine::Render::State
{
    // D3D11 objects of the game swap chain and the render flags.
    // note: The flags and the size are atomic, the D3D11 objects are only touched from the render thread.
    class RenderStore
    {
    private:
        template <typename T>
        using ComPtr = Microsoft::WRL::ComPtr<T>;

    public:
        RenderStore() = default;
        ~RenderStore() = default;

        RenderStore(const RenderStore&) = delete;
        auto operator=(const RenderStore&) -> RenderStore& = delete;

        // Counts a call inside a render hook while it lives. WaitForIdle waits for the count to reach zero.
        class HookScope
        {
        public:
            explicit HookScope(RenderStore& store) : m_Store(store)
            {
                m_Store.m_InFlight.fetch_add(1, std::memory_order_acq_rel);
            }
            ~HookScope()
            {
                m_Store.m_InFlight.fetch_sub(1, std::memory_order_acq_rel);
            }

            HookScope(const HookScope&) = delete;
            auto operator=(const HookScope&) -> HookScope& = delete;

        private:
            RenderStore& m_Store;
        };

        auto GetDevice() const -> ID3D11Device*;
        auto GetContext() const -> ID3D11DeviceContext*;
        auto GetWindow() const -> HWND;
        auto GetRenderTarget() const -> ID3D11RenderTargetView*;
        auto GetDepthStencil() const -> ID3D11DepthStencilView*;

        // return: The swap chain the store is bound to. Only for comparing, never dereference it.
        auto GetSwapChain() const -> const void*;

        auto GetWidth() const -> std::uint32_t;
        auto GetHeight() const -> std::uint32_t;
        auto GetInFlight() const -> int;

        // return: True while there is a render target. False during a resize.
        auto IsReady() const -> bool;
        auto IsResizing() const -> bool;
        auto IsShutdownRequested() const -> bool;
        auto IsShutdownDone() const -> bool;

        auto SetDevice(ComPtr<ID3D11Device> device,
            ComPtr<ID3D11DeviceContext> context, HWND window,
            const void* swapChain) -> void;

        auto SetTargets(ComPtr<ID3D11RenderTargetView> renderTarget,
            ComPtr<ID3D11DepthStencilView> depthStencil,
            std::uint32_t width, std::uint32_t height) -> void;

        auto SetResizing(bool isResizing) -> void;
        auto SetShutdownDone() -> void;
        auto RequestShutdown() -> void;

        // return: Whether the game HUD passes are skipped (VW.6.x). Read on the render thread by the HUD detours.
        auto IsGameHudHidden() const -> bool;
        auto SetGameHudHidden(bool isHidden) -> void;

        // return: Whether the game's first person arms and weapon are skipped (D331). Read from any thread by the first person detour.
        auto IsFirstPersonHidden() const -> bool;
        auto SetFirstPersonHidden(bool isHidden) -> void;

        auto SetFirstPersonHooksInstalled(bool isModelHooked, bool isUnitHooked) -> void;
        auto IsFirstPersonModelHooked() const -> bool;
        auto IsFirstPersonUnitHooked() const -> bool;

        // Counts one answer of the first person test turned to false by Drive. Any thread, no allocation.
        auto CountThirdPerson() -> void;

        // return: Whether the last completed frame (closed by EndHudFrame) turned the first person test to false at least once.
        auto WasThirdPersonLastFrame() const -> bool;

        // Thread of Present, noted at every Present. 0 until the first one.
        auto GetPresentThread() const -> std::uint32_t;
        auto SetPresentThread(std::uint32_t threadId) -> void;

        auto SetHudHookInstalled(bool isInstalled) -> void;

        // Counts one draw of the HUD for user 0 in the frame in progress.
        auto CountHudCall() -> void;

        // Closes the frame at Present: the count becomes the calls of the last frame and starts again.
        auto EndHudFrame() -> void;

        // Counts an exception that the HUD draw handling caught. note: No allocation, safe in a catch.
        auto CountHudFailure() -> void;
        auto GetHudFailures() const -> std::uint32_t;

        // Ends the Present frame in progress: the next one gets the next index and the status of this one becomes the last completed.
        // note: Called by the Present hook after the frame handlers.
        auto NextPresentFrame() -> void;

        // Records that something was drawn before the HUD in the Present frame in progress.
        auto MarkBeforeHud() -> void;

        // return: Whether something was drawn before the HUD in the Present frame in progress. The Present handlers skip that work then.
        auto IsBeforeHudMarked() const -> bool;

        // Keeps why the frame in progress did or did not draw before the HUD. A later call in the same frame replaces it.
        auto SetBeforeHudStatus(BeforeHudStatus status) -> void;

        // return: The status of the last completed Present frame.
        auto GetBeforeHudStatus() const -> BeforeHudStatus;

        // Keeps the measure of the last HUD draw. note: A short lock, no allocation. CallsPerFrame and IsHooked are ignored here.
        auto SetHudSample(const HudHookInfo& sample) -> void;

        // return: The last measure with the counters filled in.
        auto GetHudHook() const -> HudHookInfo;

        // Releases the render target and the depth stencil. Keeps the device and the context.
        auto ReleaseTargets() -> void;

        // Releases everything and clears the size.
        auto Cleanup() -> void;

    private:
        ComPtr<ID3D11Device> m_Device{};
        ComPtr<ID3D11DeviceContext> m_Context{};
        ComPtr<ID3D11RenderTargetView> m_RenderTarget{};
        ComPtr<ID3D11DepthStencilView> m_DepthStencil{};
        HWND m_Window{ nullptr };
        const void* m_SwapChain{ nullptr };

        std::atomic<std::uint32_t> m_Width{ 0 };
        std::atomic<std::uint32_t> m_Height{ 0 };

        std::atomic<bool> m_IsResizing{ false };
        std::atomic<bool> m_IsShutdownRequested{ false };
        std::atomic<bool> m_IsShutdownDone{ false };
        std::atomic<int> m_InFlight{ 0 };

        std::atomic<bool> m_IsGameHudHidden{ false };
        std::atomic<bool> m_IsHudHookInstalled{ false };
        std::atomic<bool> m_IsFirstPersonHidden{ false };
        std::atomic<bool> m_IsFirstPersonModelHooked{ false };
        std::atomic<bool> m_IsFirstPersonUnitHooked{ false };
        std::atomic<std::uint32_t> m_ThirdPersonCalls{ 0 };
        std::atomic<std::uint32_t> m_ThirdPersonLastFrame{ 0 };
        std::atomic<std::uint32_t> m_PresentThread{ 0 };
        std::atomic<std::uint32_t> m_HudCalls{ 0 };
        std::atomic<std::uint32_t> m_HudCallsLastFrame{ 0 };
        std::atomic<std::uint32_t> m_HudFailures{ 0 };

        // Number of Present frames completed, which is the index of the one in progress. The mark holds that index plus one (0 = never).
        std::atomic<std::uint64_t> m_PresentIndex{ 0 };
        std::atomic<std::uint64_t> m_BeforeHudMark{ 0 };
        std::atomic<BeforeHudStatus> m_BeforeHudStatus{ BeforeHudStatus::NoHudDraw };
        std::atomic<BeforeHudStatus> m_BeforeHudLastStatus{ BeforeHudStatus::NoHudDraw };

        mutable std::mutex m_HudSampleMutex{};
        HudHookInfo m_HudSample{};
    };
}