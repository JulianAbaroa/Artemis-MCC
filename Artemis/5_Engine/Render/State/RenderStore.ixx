module;

#include <d3d11.h>
#include <wrl/client.h>

export module Engine.Render.State;

import Engine.Render.Type;
import std;

namespace
{
    using Engine::Render::Type::DrawPlace;
    using Engine::Render::Type::MaskPairKind;
    using Engine::Render::Type::MaskTextures;
    using Engine::Render::Type::SceneStatus;

    using Engine::Render::Type::k_MaskPairCount;
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

        // return: Whether the game HUD passes are skipped. Read on the render thread by the HUD detours.
        auto IsGameHudHidden() const -> bool;
        auto SetGameHudHidden(bool isHidden) -> void;

        // return: Whether the game's first person arms and weapon are skipped (D331). Read from any thread by the first person detour.
        auto IsFirstPersonHidden() const -> bool;
        auto SetFirstPersonHidden(bool isHidden) -> void;

        auto SetFirstPersonHooksInstalled(bool isModelHooked, bool isUnitHooked, bool isDepthPassHooked, bool isOpaqueHooked) -> void;
        auto IsFirstPersonModelHooked() const -> bool;
        auto IsFirstPersonUnitHooked() const -> bool;
        auto IsFirstPersonDepthPassHooked() const -> bool;
        auto IsOpaquePassHooked() const -> bool;

        // Counts one answer of the first person test turned to false by Drive. Any thread, no allocation.
        auto CountThirdPerson() -> void;

        // return: Whether the last completed frame (closed by EndFrame) turned the first person test to false at least once.
        auto WasThirdPersonLastFrame() const -> bool;

        // Counts one answer true of the first person test for user 0, as the game receives it (Drive turns it to false, those are not counted).
        // note: Any thread, no allocation. The game hides the unit of the player when it answers true.
        auto CountFirstPersonUnit() -> void;

        // return: Whether the last completed frame (closed by EndFrame) got at least one true answer of the first person test for user 0.
        auto WasFirstPersonUnitLastFrame() const -> bool;

        // Overlay mask of the first person arms (VW.6.y.2 B). The Viewer requests it, any thread.
        auto IsFirstPersonMaskRequested() const -> bool;
        auto SetFirstPersonMaskRequested(bool isRequested) -> void;

        // Takes the one copy of a pair in the frame in progress. Any thread, no allocation.
        // return: True for the first caller of the frame for that pair, false for the others (the frame is closed by EndFrame).
        auto TakeMaskCopy(MaskPairKind kind) -> bool;

        // Notes that both copies of the pair in the frame in progress were made. Any thread.
        auto MarkMaskCopied(MaskPairKind kind) -> void;

        // return: Whether the copies of the pair were made in the frame in progress (the overlay drawn at the scene point) or in the last completed
        // one (what runs at Present).
        auto IsMaskCopied(MaskPairKind kind) const -> bool;
        auto WasMaskCopiedLastFrame(MaskPairKind kind) const -> bool;

        // Keeps the description of the texture a pair needs when its copies could not be taken (no copies yet or a different texture), so Present
        // makes them from it. note: Thread of Present only.
        auto NoteMaskDesc(MaskPairKind kind, const D3D11_TEXTURE2D_DESC& desc) -> void;

        // return: The last noted description of the pair, nothing when none was noted since the textures were released.
        auto GetMaskWantedDesc(MaskPairKind kind) const -> std::optional<D3D11_TEXTURE2D_DESC>;

        // Keeps the two textures of a pair (null when the creation failed) with their shader resource views and the description they were
        // attempted from, so the creation is not tried again for the same description. note: Thread of Present only.
        auto SetMaskTextures(MaskPairKind kind, ComPtr<ID3D11Texture2D> pre, ComPtr<ID3D11Texture2D> post, ComPtr<ID3D11ShaderResourceView> preView,
            ComPtr<ID3D11ShaderResourceView> postView, const D3D11_TEXTURE2D_DESC& desc) -> void;

        // return: Whether a creation was attempted for the pair and the textures not released since.
        auto IsMaskAttempted(MaskPairKind kind) const -> bool;

        // return: Whether both textures and both views of the pair exist.
        auto AreMaskTexturesReady(MaskPairKind kind) const -> bool;

        // return: The description of the last attempt of the pair.
        auto GetMaskDesc(MaskPairKind kind) const -> D3D11_TEXTURE2D_DESC;

        // return: Not owned. Null when the textures do not exist.
        auto GetMaskPre(MaskPairKind kind) const -> ID3D11Texture2D*;
        auto GetMaskPost(MaskPairKind kind) const -> ID3D11Texture2D*;
        auto GetMaskPreView(MaskPairKind kind) const -> ID3D11ShaderResourceView*;
        auto GetMaskPostView(MaskPairKind kind) const -> ID3D11ShaderResourceView*;

        // Keeps a reference to the resource copied at the entry of the call, so the exit copies the same one. Releases it when null.
        auto SetMaskSource(MaskPairKind kind, ID3D11Resource* source) -> void;
        auto GetMaskSource(MaskPairKind kind) const -> ID3D11Resource*;

        // Keeps a reference to the depth stencil resource of the first call with pass 0xB of the frame, for the call with pass 0 when it has none
        // bound (VW.6.y.2 E). Released when the frame closes. Releases it when null.
        auto SetMaskFrameDepth(ID3D11Resource* resource) -> void;
        auto GetMaskFrameDepth() const -> ID3D11Resource*;

        // Drops the textures, the views, the sources and the noted descriptions of every pair. Also done when the option goes off, the targets
        // are released and at cleanup.
        auto ReleaseMaskTextures() -> void;

        // The same for one pair. Dropping the pair 0xB also drops the resource kept for the pass 0 pair.
        auto ReleaseMaskTextures(MaskPairKind kind) -> void;

        // Thread of Present, noted at every Present. 0 until the first one.
        auto GetPresentThread() const -> std::uint32_t;
        auto SetPresentThread(std::uint32_t threadId) -> void;

        // Closes the frame at Present: the first person test counts become the ones of the last frame and start again, and the mask copy of the frame is closed too.
        auto EndFrame() -> void;

        // Ends the Present frame in progress: the next one gets the next index and the status of this one becomes the last completed.
        // note: Called by the Present hook after the frame handlers.
        auto NextPresentFrame() -> void;

        // Whether the Viewer wants the scene draw. The Viewer sets it at each Present from its active state, any thread, no allocation. While false the
        // scene point returns before it measures the target or builds a context.
        auto IsSceneDrawRequested() const -> bool;
        auto SetSceneDrawRequested(bool isRequested) -> void;

        // Takes the one scene point of the Present frame in progress (the first exit of the opaque color pass 0x26E500 that passes the scene blocker). Any thread, no allocation.
        // return: True for the first caller of the frame, false for the others.
        auto TakeSceneCall() -> bool;

        // Keeps why the frame in progress did or did not draw in the scene (Drawn when it did). Replaced by a later call in the same frame.
        auto SetSceneStatus(SceneStatus status) -> void;

        // return: The scene status of the last completed Present frame.
        auto GetSceneStatus() const -> SceneStatus;

        // Keeps where the frame in progress was drawn. None until the scene point draws.
        auto SetDrawPlace(DrawPlace place) -> void;

        // return: Where the last completed Present frame was drawn.
        auto GetDrawPlace() const -> DrawPlace;

        // Counts an exception at the scene point, which the call catches. Any thread.
        auto CountSceneFailure() -> void;
        auto GetSceneFailures() const -> std::uint32_t;

        // Scene color scale. The Viewer sets the slider at the start of each Present frame, any thread.
        auto SetSceneColorSlider(float slider) -> void;
        auto GetSceneColorSlider() const -> float;

        // The scale of the last scene draw (1 before the first), for the telemetry.
        auto SetSceneColorScale(float scale) -> void;
        auto GetSceneColorScale() const -> float;

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
        std::atomic<bool> m_IsFirstPersonHidden{ false };
        std::atomic<bool> m_IsFirstPersonModelHooked{ false };
        std::atomic<bool> m_IsFirstPersonUnitHooked{ false };
        std::atomic<bool> m_IsFirstPersonDepthPassHooked{ false };
        std::atomic<bool> m_IsOpaquePassHooked{ false };
        std::atomic<std::uint32_t> m_ThirdPersonCalls{ 0 };
        std::atomic<std::uint32_t> m_ThirdPersonLastFrame{ 0 };
        std::atomic<std::uint32_t> m_FirstPersonUnitCalls{ 0 };
        std::atomic<std::uint32_t> m_FirstPersonUnitLastFrame{ 0 };
        std::atomic<std::uint32_t> m_PresentThread{ 0 };

        // Number of Present frames completed, which is the index of the one in progress. The scene call mark holds that index plus one (0 = never).
        std::atomic<std::uint64_t> m_PresentIndex{ 0 };
        std::atomic<bool> m_IsSceneDrawRequested{ false };
        std::atomic<std::uint64_t> m_SceneCallMark{ 0 };
        std::atomic<SceneStatus> m_SceneStatus{ SceneStatus::NoSceneCall };
        std::atomic<SceneStatus> m_SceneLastStatus{ SceneStatus::NoSceneCall };
        std::atomic<DrawPlace> m_DrawPlace{ DrawPlace::None };
        std::atomic<DrawPlace> m_DrawPlaceLast{ DrawPlace::None };
        std::atomic<std::uint32_t> m_SceneFailures{ 0 };
        std::atomic<float> m_SceneColorSlider{ 1.0f };
        std::atomic<float> m_SceneColorScale{ 1.0f };

        std::atomic<bool> m_IsMaskRequested{ false };
        std::array<std::atomic<bool>, k_MaskPairCount> m_IsMaskTaken{};
        std::array<std::atomic<bool>, k_MaskPairCount> m_IsMaskCopied{};
        std::array<std::atomic<bool>, k_MaskPairCount> m_IsMaskCopiedLastFrame{};

        // Thread of Present only
        std::array<MaskTextures, k_MaskPairCount> m_Masks{};
        ComPtr<ID3D11Resource> m_MaskFrameDepth{};

    };
}