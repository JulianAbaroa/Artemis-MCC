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
        using DepthDiff = Engine::Render::Type::DepthDiff;
        using DepthProbeInfo = Engine::Render::Type::DepthProbeInfo;
        using FirstPersonCaller = Engine::Render::Type::FirstPersonCaller;
        using HudHookInfo = Engine::Render::Type::HudHookInfo;
        using StencilProbeInfo = Engine::Render::Type::StencilProbeInfo;
        using StencilRegion = Engine::Render::Type::StencilRegion;
        using StencilRegionStats = Engine::Render::Type::StencilRegionStats;
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

        // Runs at the entry of each call of the first person G-buffer pass (0x26E2A0) while the probe is armed and the depth pairs exist
        // (VW.6.y.2 A4): copies the bound depth stencil to the before texture of the next pair and notes the call. When no depth stencil is
        // bound it copies the resource captured from the model draw and notes that.
        // return: The pair to give to EndDepthPair after the original returns, nothing when no copy was made (probe not armed, other
        // thread, resizing, copies not ready, pairs taken, or a resource that does not match the copies).
        // note: Called by the first person detour. The immediate context is only touched on the thread of Present. COM calls only, no
        // allocation, no log.
        auto BeginDepthPair() -> std::optional<std::size_t>;

        // Copies the same resource to the after texture of the pair, once the original returned. Same rules as BeginDepthPair.
        auto EndDepthPair(std::size_t pair) -> void;

        // Compares the depth (float in bytes 0..3 of each 8 byte texel) of two R32G8X24 textures of the same size, rows pitch bytes apart
        // (VW.6.y.2 A3). A texel counts as changed when the 4 bytes differ.
        // return: Count, box, depth range of the later texture, weapon count and right half count, or nothing when a pointer is null, a size is 0 or a pitch
        // is smaller than a row.
        static auto CompareDepth(const std::uint8_t* before, std::uint32_t beforePitch, const std::uint8_t* after, std::uint32_t afterPitch,
            std::uint32_t width, std::uint32_t height) -> std::optional<DepthDiff>;

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

        auto MakeContext() const -> FrameContext;

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

        // Reads the pairs one texture at a time through the staging and reduces them. A step that fails is kept as status and HRESULT.
        // note: Allocates and blocks until the GPU finishes, once per probe, at Present.
        auto ReadDepthPairs() -> DepthProbeInfo;

        // Reads the pairs, logs one line per pair and one between each two, and releases the copies. Runs at Present, never in a detour.
        auto ReportDepthPairs() -> void;

        // Logs one INFO line per first person caller that had calls in the last frame and one for the HUD draw (VW.6.y.2).
        // note: Runs at Present, never in a detour.
        auto ReportFirstPersonProbe() -> void;

        auto Raise(const std::vector<FrameHandler>& handlers, const char* stage) -> void;

        // Logs the first 5 handler errors only.
        // note: Never throws.
        auto ReportHandlerError(const char* stage) noexcept -> void;
    };
}