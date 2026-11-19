module;

#include <d3d11.h>
#include <wrl/client.h>

export module Engine.Render.State;

import Engine.Render.Type;
import std;

namespace
{
    using Engine::Render::Type::BeforeHudStatus;
    using Engine::Render::Type::ColorPairNote;
    using Engine::Render::Type::CopyPairSetup;
    using Engine::Render::Type::DepthPairNote;
    using Engine::Render::Type::FirstPersonCaller;
    using Engine::Render::Type::FirstPersonSample;
    using Engine::Render::Type::HudHookInfo;
    using Engine::Render::Type::MaskWindow;
    using Engine::Render::Type::MaskWindowReport;
    using Engine::Render::Type::StencilProbeInfo;

    using Engine::Render::Type::k_ColorPairCount;
    using Engine::Render::Type::k_DepthPairCount;
    using Engine::Render::Type::k_FirstPersonCallerCount;
}

export namespace Engine::Render::State
{
    // The copy textures of a probe (VW.6.y.2 A3, D): Count pairs of before and after (pair k is copy 2k before and 2k + 1 after), the staging
    // texture to read them, the description they were made from and the calls noted in call order with the resource each one copied from.
    // note: Touched on the thread of Present only (the detour copies there too), no lock. Used for the depth pairs and the color pairs.
    template <typename Note, std::size_t Count>
    class CopyPairs
    {
    private:
        template <typename T>
        using ComPtr = Microsoft::WRL::ComPtr<T>;

    public:
        auto Set(std::array<ComPtr<ID3D11Texture2D>, Count * 2> copies, ComPtr<ID3D11Texture2D> staging, const D3D11_TEXTURE2D_DESC& desc) -> void
        {
            m_Copies = std::move(copies);
            m_Staging = std::move(staging);
            m_Desc = desc;
        }

        // return: Whether the copies exist.
        auto AreReady() const -> bool
        {
            return m_Staging != nullptr;
        }

        // How the creation went. Kept even when it failed, so the probe tries once.
        auto SetSetup(const CopyPairSetup& setup) -> void
        {
            m_Setup = setup;
        }

        auto GetSetup() const -> CopyPairSetup
        {
            return m_Setup;
        }

        // return: The description of the original texture the copies were made from.
        auto GetDesc() const -> D3D11_TEXTURE2D_DESC
        {
            return m_Desc;
        }

        // return: Copy texture i (not owned), null when i is out of range or none exist.
        auto GetCopy(std::size_t index) const -> ID3D11Texture2D*
        {
            return index < m_Copies.size() ? m_Copies[index].Get() : nullptr;
        }

        auto GetStaging() const -> ID3D11Texture2D*
        {
            return m_Staging.Get();
        }

        // Appends a call to the list and keeps a reference to the resource it copied from, so the after copy uses the same one.
        // param source: The resource that was copied, null for a call that copied nothing.
        // return: Its position, or nothing when the Count pairs are taken. No allocation.
        auto NoteCall(const Note& note, ID3D11Resource* source) -> std::optional<std::size_t>
        {
            if (m_NoteCount >= Count) return std::nullopt;

            const std::size_t index{ m_NoteCount++ };
            m_Notes[index] = note;
            m_Sources[index] = source;
            return index;
        }

        auto GetNote(std::size_t index) const -> Note
        {
            return index < m_NoteCount ? m_Notes[index] : Note{};
        }

        auto GetNoteCount() const -> std::uint32_t
        {
            return m_NoteCount;
        }

        auto GetSource(std::size_t index) const -> ID3D11Resource*
        {
            return index < m_NoteCount ? m_Sources[index].Get() : nullptr;
        }

        // Drops the copies, the staging, the notes and the setup.
        auto Release() -> void
        {
            for (auto& copy : m_Copies) copy.Reset();
            for (auto& source : m_Sources) source.Reset();

            m_Staging.Reset();
            m_Desc = {};
            m_Setup = {};
            m_Notes = {};
            m_NoteCount = 0;
        }

    private:
        std::array<ComPtr<ID3D11Texture2D>, Count * 2> m_Copies{};
        ComPtr<ID3D11Texture2D> m_Staging{};
        D3D11_TEXTURE2D_DESC m_Desc{};
        CopyPairSetup m_Setup{};
        std::array<Note, Count> m_Notes{};
        std::array<ComPtr<ID3D11Resource>, Count> m_Sources{};
        std::uint32_t m_NoteCount{ 0 };
    };

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

        auto SetFirstPersonHooksInstalled(bool isModelHooked, bool isUnitHooked, bool isDepthPassHooked, bool isForwardHooked) -> void;
        auto IsFirstPersonModelHooked() const -> bool;
        auto IsFirstPersonUnitHooked() const -> bool;
        auto IsFirstPersonDepthPassHooked() const -> bool;
        auto IsFirstPersonForwardHooked() const -> bool;

        // Counts one answer of the first person test turned to false by Drive. Any thread, no allocation.
        auto CountThirdPerson() -> void;

        // return: Whether the last completed frame (closed by EndHudFrame) turned the first person test to false at least once.
        auto WasThirdPersonLastFrame() const -> bool;

        // Counts one answer true of the first person test for user 0, as the game receives it (Drive turns it to false, those are not counted).
        // note: Any thread, no allocation. The game hides the unit of the player when it answers true.
        auto CountFirstPersonUnit() -> void;

        // return: Whether the last completed frame (closed by EndHudFrame) got at least one true answer of the first person test for user 0.
        auto WasFirstPersonUnitLastFrame() const -> bool;

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

        // Depth pairs of the probe (VW.6.y.2 A3): textures that hold the depth before and after each call of the first person pass 0x26DA08.
        // note: Dropped (Release) when a probe is armed, the targets are released and at cleanup, and by the report at the end of the probe.
        auto GetDepthPairs() -> CopyPairs<DepthPairNote, k_DepthPairCount>&;
        auto GetDepthPairs() const -> const CopyPairs<DepthPairNote, k_DepthPairCount>&;

        // Color pairs of the probe (VW.6.y.2 D): the same for the render target 0 around each call of the forward pass 0x26EA78.
        auto GetColorPairs() -> CopyPairs<ColorPairNote, k_ColorPairCount>&;
        auto GetColorPairs() const -> const CopyPairs<ColorPairNote, k_ColorPairCount>&;

        // Keeps the description of the render target 0 the forward pass was drawn with when the color pairs did not exist yet, so Present makes
        // them from it. Does nothing when one is noted. note: Thread of Present only, a plain struct, no reference is held.
        auto NoteColorDesc(const D3D11_TEXTURE2D_DESC& desc) -> void;

        // return: The noted description, nothing when none was noted since the color pairs were released.
        auto GetColorDesc() const -> std::optional<D3D11_TEXTURE2D_DESC>;

        // Drops the color pairs and the noted description. Same moments as the depth pairs.
        auto ReleaseColorPairs() -> void;

        // Overlay mask of the first person arms (VW.6.y.2 B). The Viewer requests it with the option Arms over overlay, any thread.
        auto IsFirstPersonMaskRequested() const -> bool;
        auto SetFirstPersonMaskRequested(bool isRequested) -> void;

        // Takes the one mask copy of the frame in progress. Any thread, no allocation.
        // return: True for the first caller of the frame, false for the others (the frame is closed by EndHudFrame).
        auto TakeMaskCopy() -> bool;

        // Notes that both copies of the frame in progress were made. Any thread.
        auto MarkMaskCopied() -> void;

        // return: Whether the copies were made in the frame in progress (the overlay drawn before the HUD) or in the last completed one
        // (the overlay drawn at Present).
        auto IsMaskCopied() const -> bool;
        auto WasMaskCopiedLastFrame() const -> bool;

        // Counts a call of the first person pass 0xB that asked for the mask, whether or not it copied (VW.6.y.2 C). Any thread, no allocation.
        auto CountMaskPassCall() -> void;

        // return: The last closed telemetry window of the mask, empty before the first one or after the mask stops being requested.
        // note: The window closes in EndHudFrame every k_MaskWindowFrames frames with the mask requested.
        auto GetMaskWindowReport() const -> std::optional<MaskWindowReport>;

        // Keeps the description of the depth texture the first person pass wrote to when the copies could not be taken (no copies yet or a
        // different texture), so Present makes them from it. note: Thread of Present only.
        auto NoteMaskDepthDesc(const D3D11_TEXTURE2D_DESC& desc) -> void;

        // return: The last noted description, nothing when none was noted since the textures were released.
        auto GetMaskDepthDesc() const -> std::optional<D3D11_TEXTURE2D_DESC>;

        // Keeps the two mask textures (null when the creation failed) with their shader resource views and the description they were
        // attempted from, so the creation is not tried again for the same description. note: Thread of Present only.
        auto SetMaskTextures(ComPtr<ID3D11Texture2D> pre, ComPtr<ID3D11Texture2D> post, ComPtr<ID3D11ShaderResourceView> preView,
            ComPtr<ID3D11ShaderResourceView> postView, const D3D11_TEXTURE2D_DESC& desc) -> void;

        // return: Whether a creation was attempted and the textures not released since.
        auto IsMaskAttempted() const -> bool;

        // return: Whether both textures and both views exist.
        auto AreMaskTexturesReady() const -> bool;

        // return: The description of the last attempt.
        auto GetMaskDesc() const -> D3D11_TEXTURE2D_DESC;

        // return: Not owned. Null when the textures do not exist.
        auto GetMaskPre() const -> ID3D11Texture2D*;
        auto GetMaskPost() const -> ID3D11Texture2D*;
        auto GetMaskPreView() const -> ID3D11ShaderResourceView*;
        auto GetMaskPostView() const -> ID3D11ShaderResourceView*;

        // Keeps a reference to the resource copied at the entry of the pass, so the exit copies the same one. Releases it when null.
        auto SetMaskSource(ID3D11Resource* source) -> void;
        auto GetMaskSource() const -> ID3D11Resource*;

        // Drops the textures, the views, the source and the noted description. Also done when the option goes off, the targets are
        // released and at cleanup.
        auto ReleaseMaskTextures() -> void;

        // Thread of Present, noted at every Present. 0 until the first one.
        auto GetPresentThread() const -> std::uint32_t;
        auto SetPresentThread(std::uint32_t threadId) -> void;

        auto SetHudHookInstalled(bool isInstalled) -> void;

        // Counts one draw of the HUD for user 0 in the frame in progress.
        auto CountHudCall() -> void;

        // Closes the frame at Present: the count becomes the calls of the last frame and starts again. The mask copy of the frame is closed too.
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
        std::atomic<bool> m_IsFirstPersonDepthPassHooked{ false };
        std::atomic<bool> m_IsFirstPersonForwardHooked{ false };
        std::atomic<std::uint32_t> m_ThirdPersonCalls{ 0 };
        std::atomic<std::uint32_t> m_ThirdPersonLastFrame{ 0 };
        std::atomic<std::uint32_t> m_FirstPersonUnitCalls{ 0 };
        std::atomic<std::uint32_t> m_FirstPersonUnitLastFrame{ 0 };
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

        std::atomic<bool> m_IsMaskRequested{ false };
        std::atomic<bool> m_IsMaskCallTaken{ false };
        std::atomic<bool> m_IsMaskCopied{ false };
        std::atomic<bool> m_IsMaskCopiedLastFrame{ false };
        std::atomic<std::uint32_t> m_MaskPassCalls{ 0 };
        MaskWindow m_MaskWindow{};      // thread of Present only (EndHudFrame)
        std::atomic<bool> m_IsMaskWindowPublished{ false };
        std::atomic<std::uint32_t> m_MaskWindowFrames{ 0 };
        std::atomic<std::uint32_t> m_MaskWindowNoCopy{ 0 };
        std::atomic<std::uint32_t> m_MaskWindowMaxCalls{ 0 };

        // Thread of Present only
        ComPtr<ID3D11Texture2D> m_MaskPre{};
        ComPtr<ID3D11Texture2D> m_MaskPost{};
        ComPtr<ID3D11ShaderResourceView> m_MaskPreView{};
        ComPtr<ID3D11ShaderResourceView> m_MaskPostView{};
        ComPtr<ID3D11Resource> m_MaskSource{};
        D3D11_TEXTURE2D_DESC m_MaskDesc{};
        std::optional<D3D11_TEXTURE2D_DESC> m_MaskDepthDesc{};
        bool m_IsMaskAttempted{ false };

        CopyPairs<DepthPairNote, k_DepthPairCount> m_DepthPairs{};  // Thread of Present only
        CopyPairs<ColorPairNote, k_ColorPairCount> m_ColorPairs{};
        std::optional<D3D11_TEXTURE2D_DESC> m_ColorDesc{};
    };
}