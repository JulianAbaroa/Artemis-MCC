module;

#include <d3d11.h>
#include <dxgi.h>
#include <wrl/client.h>

export module Engine.Render.System:RenderService;

import Common.Log.System;
import Engine.Render.Type;
import Engine.Render.State;
import std;

export namespace Engine::Render::System
{
    // Runs the render frame from the Present hook and raises the render events to the handlers.
    // It binds to the swap chain, recreates the targets on resize and runs the shutdown.
    // note: The events run on the game render thread. Handlers that throw are logged and skipped.
    class RenderService
    {
    private:
        template <typename T>
        using ComPtr = Microsoft::WRL::ComPtr<T>;

        using milliseconds = std::chrono::milliseconds;

        using Log = Common::Log::System::Log;

        using FrameContext = Engine::Render::Type::FrameContext;
        using FrameHandler = Engine::Render::Type::FrameHandler;
        using BeforeHudHandler = Engine::Render::Type::BeforeHudHandler;
        using BeforeHudStatus = Engine::Render::Type::BeforeHudStatus;
        using ChangedMap = Engine::Render::Type::ChangedMap;
        using ColorProbeInfo = Engine::Render::Type::ColorProbeInfo;
        using MaskContext = Engine::Render::Type::MaskContext;
        using MaskPairKind = Engine::Render::Type::MaskPairKind;
        using MaskPlan = Engine::Render::Type::MaskPlan;
        using DepthProbeInfo = Engine::Render::Type::DepthProbeInfo;
        using FirstPersonCaller = Engine::Render::Type::FirstPersonCaller;
        using HudHookInfo = Engine::Render::Type::HudHookInfo;
        using StencilProbeInfo = Engine::Render::Type::StencilProbeInfo;
        using StencilRegion = Engine::Render::Type::StencilRegion;
        using StencilRegionStats = Engine::Render::Type::StencilRegionStats;
        using TexelDiff = Engine::Render::Type::TexelDiff;
        using TexelLayout = Engine::Render::Type::TexelLayout;
        using ShutdownCallback = Engine::Render::Type::ShutdownCallback;

        using RenderStore = Engine::Render::State::RenderStore;

    public:
        RenderService(Log& log, RenderStore& renderStore) :
            m_Log(log), m_RenderStore(renderStore) {}
        ~RenderService() = default;

        RenderService(const RenderService&) = delete;
        auto operator=(const RenderService&) -> RenderService& = delete;

        // Handlers are called in registration order.
        auto OnInitialized(FrameHandler handler) -> void;   // first bind to a swap chain
        auto OnFrame(FrameHandler handler) -> void;         // every frame
        auto OnPostFrame(FrameHandler handler) -> void;     // every frame, after OnFrame
        auto OnResize(FrameHandler handler) -> void;        // after the targets are recreated

        // Registers a handler that draws under the HUD of the game, called from the HUD draw before the original (VW.6.x B).
        // note: Runs on the game render thread inside a GpuStateGuard. It returns whether it drew.
        auto OnBeforeHud(BeforeHudHandler handler) -> void;

        // Callbacks are called in reverse registration order.
        auto OnShutdown(ShutdownCallback callback) -> void;

        // Runs one frame. Skipped while resizing. Runs the shutdown if it was requested.
        // note: Called by the Present hook.
        auto PresentFrame(IDXGISwapChain* swapChain) -> void;

        // Releases the targets so the swap chain can resize.
        // note: Called by the ResizeBuffers hook before the original.
        auto BeginResize() -> void;

        // Recreates the targets if the resize succeeded.
        // note: Called by the ResizeBuffers hook after the original.
        auto EndResize(IDXGISwapChain* swapChain, HRESULT result) -> void;

        // Asks for the shutdown. It runs on the next Present.
        auto RequestShutdown() -> void;

        // return: False if the shutdown did not finish before the timeout.
        auto WaitForShutdown(milliseconds timeout) -> bool;

        // Runs for each HUD draw of user 0, before the original: counts it, measures what the game draws its HUD with (thread, render target
        // in slot 0, viewport 0, into the store) and, when nothing blocks it, calls the before-HUD handlers inside a GpuStateGuard.
        // note: Called by the HUD detour on the game render thread. Nothing escapes: an exception is caught and counted in the store. The immediate context is only touched on the thread of Present with the
        // targets ready (COM calls only, no allocation in the measure). Each Present frame runs the handlers once and marks the frame in
        // the store when one drew.
        auto OnHudDraw() -> void;

        // return: Why the handlers must not run for this HUD draw, or nothing if they can: no thread of Present, no targets (not ready,
        // resizing or shutting down), a bound target of another device, that is not the current back buffer, of another size, or a
        // viewport 0 that is not the whole target.
        static auto FindBeforeHudBlocker(const HudHookInfo& sample, bool isReady, std::uint32_t width, std::uint32_t height)
            -> std::optional<BeforeHudStatus>;

        // Runs for each call of the first person model pass for user 0 while the probe is armed, before the original: reads the render
        // targets, depth stencil, viewport 0, depth stencil state and rasterizer state bound now, and keeps them in the slot of the caller.
        // param callerRva: The return address minus the base of the game module.
        // param pass: The uint8 argument of the model pass.
        // note: Called by the first person detour. Reads only, nothing changes what is drawn. The immediate context is only touched on the
        // thread of Present with the targets ready, on another thread the sample only says so. COM calls only, no allocation, no log.
        auto MeasureFirstPerson(FirstPersonCaller caller, std::uint32_t callerRva, std::uint8_t pass) -> void;

        // Runs at the entry of each call of the first person pass 0x26DA08 while the probe is armed and the depth pairs exist
        // (VW.6.y.2 A5): copies the bound depth stencil to the before texture of the next pair and notes the call. When no depth stencil is
        // bound it copies the resource captured from the model draw and notes that.
        // param caller: Pass1, the note keeps it.
        // param pass: The pass argument of 0x26DA08.
        // return: The pair to give to EndDepthPair after the original returns, nothing when no copy was made (probe not armed, other
        // thread, resizing, copies not ready, pairs taken, or a resource that does not match the copies).
        // note: Called by the first person detour. The immediate context is only touched on the thread of Present. COM calls only, no
        // allocation, no log.
        auto BeginDepthPair(FirstPersonCaller caller, std::int32_t pass) -> std::optional<std::size_t>;

        // Copies the same resource to the after texture of the pair, once the original returned. Same rules as BeginDepthPair.
        auto EndDepthPair(std::size_t pair) -> void;

        // Runs at the entry of each call of the forward pass 0x26EA78 while the probe is armed (VW.6.y.2 D). Without color pairs it only notes the
        // description of the render target 0 (the first time), so Present makes them. With them it copies the resource of the render target 0 to
        // the before texture of the next pair and notes the call: no render target 0 is NoTarget, another description is Mismatch.
        // param phase: The phase argument of 0x26EA78.
        // return: The pair to give to EndColorPair after the original returns, nothing when no copy was made.
        // note: Called by the first person detour. The immediate context is only touched on the thread of Present. Reads only atomics when the
        // probe is not armed. COM calls only, no allocation, no log.
        auto BeginColorPair(std::uint8_t phase) -> std::optional<std::size_t>;

        // Copies the same resource to the after texture of the pair, once the original returned.
        auto EndColorPair(std::size_t pair) -> void;

        // Runs at the entry of 0x26DA08 (VW.6.y.2 B, E). For the first call of the frame with pass 0xB (pair DepthB) and the first with pass 0 (pair
        // DepthZero), when the mask is requested and the textures of the pair exist, copies the depth stencil resource to the before texture and
        // keeps the resource for EndMaskCopy. The pass 0 call with no depth stencil bound uses the one of the pass 0xB call of the same frame, and
        // with none of them it copies nothing. When the textures do not exist or do not match the resource, the pass 0xB call notes the description
        // of the resource, so Present makes them (the pass 0 pair is made from the same description), and the frame has no mask.
        // return: The pair whose before copy was made, so the detour calls EndMaskCopy with it after the original returns. Nothing otherwise.
        // note: Called by the first person detour. The immediate context is only touched on the thread of Present. COM calls only, no
        // allocation, no log.
        auto BeginMaskCopy(std::int32_t pass) -> std::optional<MaskPairKind>;

        // Copies the resource of BeginMaskCopy to the after texture of the pair and marks the pair as copied in the frame. Same rules as
        // BeginMaskCopy.
        auto EndMaskCopy(MaskPairKind kind) -> void;

        // Runs at the entry of the forward pass 0x26EA78 (VW.6.y.2 E). For the first call of the frame with a phase other than 0, when the mask is
        // requested and the color textures exist, copies the render target 0 to the before texture. When they do not exist or do not match it
        // notes the description of the render target 0, so Present makes them.
        // return: True when the before copy was made, so the detour calls EndColorMaskCopy after the original returns.
        // note: Same rules as BeginMaskCopy.
        auto BeginColorMaskCopy(std::uint8_t phase) -> bool;

        // Copies the render target of BeginColorMaskCopy to the after texture and marks the color pair as copied in the frame.
        auto EndColorMaskCopy() -> void;

        // Decides at Present what to do with the mask textures (VW.6.y.2 B).
        // param wanted: The description of the depth texture of the first person pass, nothing when none was noted.
        // param attempted: The description the textures were last made from, only read when isAttempted.
        // return: Release when the mask is not requested and textures were made, Create when it is requested, a description is known and no
        // attempt was made for it (none yet, or another size, format or samples), Keep otherwise (a failed attempt is not repeated).
        static auto PlanMask(bool isRequested, const std::optional<D3D11_TEXTURE2D_DESC>& wanted, bool isAttempted,
            const D3D11_TEXTURE2D_DESC& attempted) -> MaskPlan;

        // return: The texel of a mask of maskSize texels that covers the pixel of a target of targetSize pixels, on one axis. The same
        // arithmetic as the map shaders. 0 when a size is 0.
        static auto ScaleToMask(std::uint32_t pixel, std::uint32_t targetSize, std::uint32_t maskSize) -> std::uint32_t;

        // Compares two textures of the same size and layout, rows pitch bytes apart (VW.6.y.2 A3, D). A texel counts as changed when its first
        // CompareBytes bytes differ.
        // param changedMap: When not null it is filled with a byte per texel, 1 where it changed.
        // return: Count, box, depth range of the later texture (0 when the layout has none), weapon count and right half count, or nothing when
        // a pointer is null, a size is 0, the layout is not valid or a pitch is smaller than a row.
        static auto CompareTexels(const std::uint8_t* before, std::uint32_t beforePitch, const std::uint8_t* after, std::uint32_t afterPitch,
            std::uint32_t width, std::uint32_t height, const TexelLayout& layout, ChangedMap* changedMap) -> std::optional<TexelDiff>;

        // CompareTexels of two R32G8X24 depth textures (float depth in bytes 0..3 of each 8 byte texel, the 4 bytes compared).
        static auto CompareDepth(const std::uint8_t* before, std::uint32_t beforePitch, const std::uint8_t* after, std::uint32_t afterPitch,
            std::uint32_t width, std::uint32_t height) -> std::optional<TexelDiff>;

        // return: How many texels changed in color are not changed in depth (VW.6.y.2 D), the two maps of the same size and complete. Nothing when
        // a map is empty, the sizes differ or a map is shorter than its size: the count is not made for sizes that differ.
        static auto CountOutsideMask(const ChangedMap& depth, const ChangedMap& color) -> std::optional<std::uint32_t>;

        // Reduces a mapped R32G8X24 depth stencil texture to the stats of its regions (VW.6.y.2 A2): 8 bytes per texel, the float depth in
        // bytes 0..3 and the stencil in byte 4, rows rowPitch bytes apart (the pitch may be larger than the row).
        // return: Status Read with the stats, or BadLayout when data is null, a size is 0 or the pitch is smaller than a row.
        static auto ReduceStencil(const std::uint8_t* data, std::uint32_t rowPitch, std::uint32_t width, std::uint32_t height)
            -> StencilProbeInfo;

        // return: One log line of a region: texel count, the 8 most frequent stencil values with their counts, texels per bit and depth range.
        static auto DescribeStencilRegion(StencilRegion region, const StencilRegionStats& stats) -> std::string;

        // Waits for the calls inside the render hooks to finish.
        // return: False if some are still running after the timeout.
        auto WaitForIdle(milliseconds timeout) -> bool;

        // Runs the shutdown callbacks and releases the store. Does nothing if already done.
        auto Shutdown() -> void;

    private:
        Log& m_Log;
        RenderStore& m_RenderStore;

        std::vector<FrameHandler> m_OnInitialized{};
        std::vector<FrameHandler> m_OnFrame{};
        std::vector<FrameHandler> m_OnPostFrame{};
        std::vector<FrameHandler> m_OnResize{};
        std::vector<BeforeHudHandler> m_OnBeforeHud{};
        std::vector<ShutdownCallback> m_OnShutdown{};

        std::mutex m_ShutdownMutex{};
        std::atomic<int> m_HandlerErrors{ 0 };
        bool m_IsAttachFailed{ false };

        // Attaches to the swap chain and raises OnInitialized the first time, OnResize otherwise.
        auto Bind(IDXGISwapChain* swapChain) -> void;

        // Takes the device and creates the targets. Logs the first failure only.
        // return: False if the device or the render target could not be obtained.
        auto Attach(IDXGISwapChain* swapChain) -> bool;

        // param isInFrame: Whether the mask is the one of the game frame in progress (drawing before the HUD) or of the last completed
        // one (drawing at Present).
        auto MakeContext(bool isInFrame) const -> FrameContext;
        auto MakeMask(bool isInFrame) const -> MaskContext;

        // At Present: makes, replaces or releases the mask textures of each pair as PlanMask says (VW.6.y.2 B, E). The result, failed or not, is
        // kept in the store, so a failed creation is logged once for a description. Thread of Present only.
        auto PrepareMask() -> void;

        // The same for one pair. The pass 0 pair takes the description of the pass 0xB pair.
        auto PrepareMaskPair(MaskPairKind kind) -> void;

        // Copies the resource to the before texture of the pair when the pair is ready and the resource has its description, and keeps the
        // resource for the exit copy.
        // param canNote: Whether a missing or different texture notes the description of the resource for Present.
        // return: True when the copy was made. False when the pair is not ready, the resource does not match or the frame already has its copy.
        auto BeginMaskPairCopy(MaskPairKind kind, ID3D11DeviceContext* context, ID3D11Resource* resource, bool canNote) -> bool;

        // The body of OnHudDraw. Exceptions are caught by the caller.
        auto HandleHudDraw() -> void;

        // Reads the render target bound in slot 0 and the viewport into a sample, and stores it.
        auto MeasureHud() -> HudHookInfo;

        // At a HUD draw of an armed probe, once: copies the depth stencil texture the first person model was drawn with to a staging texture,
        // maps it and keeps the reduced stencil in the store (VW.6.y.2 A2). Blocks until the GPU finishes the copy, accepted for the one frame
        // a probe measures. A failed step is kept as status and HRESULT. Thread of Present only (the caller checks), no log.
        auto ReadFirstPersonStencil() -> void;

        // At Present of an armed probe whose depth stencil resource is known, once: makes the depth pair textures and the staging with its
        // description (VW.6.y.2 A3). The frame of the probe that follows is the one that fills them. The result, failed or not, is kept in
        // the store. Thread of Present only.
        auto PrepareDepthPairs() -> void;

        // return: The immediate context when the detour may copy depth for this caller now, null otherwise.
        auto GetDepthPairContext() -> ID3D11DeviceContext*;

        // return: The immediate context when the call runs on the thread of Present with the store ready, null otherwise.
        auto GetPresentContext() -> ID3D11DeviceContext*;

        // At Present of an armed probe whose color target is noted, once: makes the color pair textures and the staging with that description
        // (VW.6.y.2 D), the same way as the depth pairs. A format that is not 4 bytes per texel or has samples is kept as unsupported.
        auto PrepareColorPairs() -> void;

        // Reads the pairs one texture at a time through the staging and reduces them. A step that fails is kept as status and HRESULT.
        // param mask: Receives the union of the texels that the copied pairs of the passes 0xB and 0 changed (left empty when there is none).
        // note: Allocates and blocks until the GPU finishes, once per probe, at Present.
        auto ReadDepthPairs(ChangedMap& mask) -> DepthProbeInfo;

        // The same for the color pairs. Each pair also counts its changed texels outside of the depth mask.
        auto ReadColorPairs(const ChangedMap& depthMask) -> ColorProbeInfo;

        // Reads the pairs, logs one line per pair and one between each two, and releases the copies. Runs at Present, never in a detour.
        auto ReportDepthPairs(ChangedMap& mask) -> void;

        // The same for the color pairs, one line per pair, or one line that says why there are none.
        auto ReportColorPairs(const ChangedMap& depthMask) -> void;

        // Reports the depth pairs, then the color pairs with the depth mask of the pairs 0xB and 0.
        auto ReportCopyPairs() -> void;

        // Logs one INFO line per first person caller that had calls in the last frame and one for the HUD draw (VW.6.y.2).
        // note: Runs at Present, never in a detour.
        auto ReportFirstPersonProbe() -> void;

        auto Raise(const std::vector<FrameHandler>& handlers, const char* stage) -> void;

        // Logs the first 5 handler errors only.
        // note: Never throws.
        auto ReportHandlerError(const char* stage) noexcept -> void;
    };
}