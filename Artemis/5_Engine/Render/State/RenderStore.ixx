module;

#include <d3d11.h>
#include <wrl/client.h>

export module Engine.Render.State;

import Engine.Render.Type;
import std;

namespace
{
    using Engine::Render::Type::BeforeHudStatus;
    using Engine::Render::Type::DepthPairNote;
    using Engine::Render::Type::DepthPairSetup;
    using Engine::Render::Type::FirstPersonCaller;
    using Engine::Render::Type::FirstPersonSample;
    using Engine::Render::Type::HudHookInfo;
    using Engine::Render::Type::StencilProbeInfo;

    using Engine::Render::Type::k_DepthCopyCount;
    using Engine::Render::Type::k_DepthPairCount;
    using Engine::Render::Type::k_FirstPersonCallerCount;
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

        auto SetFirstPersonHooksInstalled(bool isModelHooked, bool isUnitHooked, bool isPassHooked, bool isDepthPassHooked) -> void;
        auto IsFirstPersonModelHooked() const -> bool;
        auto IsFirstPersonUnitHooked() const -> bool;
        auto IsFirstPersonPassHooked() const -> bool;
        auto IsFirstPersonDepthPassHooked() const -> bool;

        // Counts one answer of the first person test turned to false by Drive. Any thread, no allocation.
        auto CountThirdPerson() -> void;

        // return: Whether the last completed frame (closed by EndHudFrame) turned the first person test to false at least once.
        auto WasThirdPersonLastFrame() const -> bool;

        // Counts one call of the first person model pass for user 0 in the frame in progress. Any thread, no allocation.
        auto CountFirstPersonCall(FirstPersonCaller caller) -> void;

        // return: The calls of that caller in the last completed frame (closed by EndHudFrame).
        auto GetFirstPersonCalls(FirstPersonCaller caller) const -> std::uint32_t;

        // return: How many callers had at least one call in the last completed frame.
        auto GetFirstPersonCallerCount() const -> std::uint32_t;

        // Asks for one probe of the first person model pass (VW.6.y.2). Does nothing if one is already pending. Any thread.
        auto RequestFirstPersonProbe() -> void;

        // return: Whether a probe was requested and has not been reported yet.
        auto IsFirstPersonProbePending() const -> bool;

        // return: Whether the detour must measure its calls: the probe began at a Present and its frame is in progress.
        auto IsFirstPersonProbeArmed() const -> bool;

        // Moves the probe one Present on, after EndHudFrame: a requested probe is armed (the samples are cleared, so the frame that follows
        // is measured whole) and an armed one ends when the closed frame had first person calls.
        // return: True when the frame just closed is the one to report. The probe is then idle again.
        auto StepFirstPersonProbe() -> bool;

        // Keeps the measure of a call of the first person model pass, in the slot of its caller. note: A short lock, no allocation.
        auto SetFirstPersonSample(const FirstPersonSample& sample) -> void;

        // return: The last measure of that caller with the calls of the last frame filled in.
        auto GetFirstPersonSample(FirstPersonCaller caller) const -> FirstPersonSample;

        // Keeps a reference to the resource of the depth stencil view the first person model is drawn with (the first one seen while the probe
        // is armed). Does nothing when one is held or the resource is null. note: Thread of Present only (VW.6.y.2 A2).
        auto CaptureFirstPersonDepth(ID3D11Resource* resource) -> void;

        // return: The captured resource, null when none. Not owned. Thread of Present only.
        auto GetFirstPersonDepth() const -> ID3D11Resource*;

        // Releases the captured resource. Also done when the probe ends, a new probe is armed, the targets are released and at cleanup.
        auto ReleaseFirstPersonDepth() -> void;

        // Keeps the stencil read of the probe. note: A short lock.
        auto SetStencilProbe(const StencilProbeInfo& info) -> void;

        // return: The stencil read of the probe (IsAttempted false when none).
        auto GetStencilProbe() const -> StencilProbeInfo;

        // return: Whether the stencil of this probe was already tried, so it is read once.
        auto IsStencilProbeAttempted() const -> bool;

        // Notes whether Drive was active at the last measured model call, to log it with the probe. Any thread.
        auto SetFirstPersonDriveActive(bool isActive) -> void;
        auto IsFirstPersonDriveActive() const -> bool;

        // Depth pairs of the probe (VW.6.y.2 A3): textures that hold the depth before and after each call of the first person G-buffer pass.
        // note: All of this is touched on the thread of Present only (the detour copies there too), no lock.

        // Keeps the 2 * k_DepthPairCount copy textures (pair k is 2k before, 2k + 1 after), the staging texture to read them and the
        // description they were made from.
        auto SetDepthPairs(std::array<ComPtr<ID3D11Texture2D>, k_DepthCopyCount> copies, ComPtr<ID3D11Texture2D> staging,
            const D3D11_TEXTURE2D_DESC& desc) -> void;

        // return: Whether the copies exist.
        auto AreDepthPairsReady() const -> bool;

        // How the creation went. Kept even when it failed, so the probe tries once.
        auto SetDepthPairSetup(const DepthPairSetup& setup) -> void;
        auto GetDepthPairSetup() const -> DepthPairSetup;

        // return: The description of the original depth texture the copies were made from.
        auto GetDepthPairDesc() const -> D3D11_TEXTURE2D_DESC;

        // return: Copy texture i (not owned), null when i is out of range or none exist.
        auto GetDepthCopy(std::size_t index) const -> ID3D11Texture2D*;
        auto GetDepthStaging() const -> ID3D11Texture2D*;

        // Appends a call of the pass to the list and keeps a reference to the resource it copied from, so the after copy uses the same one.
        // param source: The resource that was copied, null for a call that copied nothing.
        // return: Its position, or nothing when the k_DepthPairCount are taken. No allocation.
        auto NoteDepthPair(const DepthPairNote& note, ID3D11Resource* source) -> std::optional<std::size_t>;
        auto GetDepthPairNote(std::size_t index) const -> DepthPairNote;
        auto GetDepthPairNoteCount() const -> std::uint32_t;
        auto GetDepthPairSource(std::size_t index) const -> ID3D11Resource*;

        // Drops the copies, the staging, the notes and the setup. Also done when a probe is armed, the targets are released and at cleanup.
        auto ReleaseDepthPairs() -> void;

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
        std::atomic<bool> m_IsFirstPersonPassHooked{ false };
        std::atomic<bool> m_IsFirstPersonDepthPassHooked{ false };
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

        std::array<std::atomic<std::uint32_t>, k_FirstPersonCallerCount> m_FirstPersonCalls{};
        std::array<std::atomic<std::uint32_t>, k_FirstPersonCallerCount> m_FirstPersonCallsLastFrame{};
        std::atomic<std::uint8_t> m_FirstPersonProbe{ 0 };

        mutable std::mutex m_FirstPersonSampleMutex{};
        std::array<FirstPersonSample, k_FirstPersonCallerCount> m_FirstPersonSamples{};

        ComPtr<ID3D11Resource> m_FirstPersonDepth{};    // Thread of Present only
        std::atomic<bool> m_IsStencilAttempted{ false };
        std::atomic<bool> m_IsFirstPersonDriveActive{ false };
        mutable std::mutex m_StencilMutex{};
        StencilProbeInfo m_StencilProbe{};

        std::array<ComPtr<ID3D11Texture2D>, k_DepthCopyCount> m_DepthCopies{};  // Thread of Present only
        ComPtr<ID3D11Texture2D> m_DepthStaging{};
        D3D11_TEXTURE2D_DESC m_DepthPairDesc{};
        DepthPairSetup m_DepthPairSetup{};
        std::array<DepthPairNote, k_DepthPairCount> m_DepthNotes{};
        std::array<ComPtr<ID3D11Resource>, k_DepthPairCount> m_DepthSources{};
        std::uint32_t m_DepthNoteCount{ 0 };
    };
}