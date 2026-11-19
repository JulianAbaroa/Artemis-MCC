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
        using SceneHandler = Engine::Render::Type::SceneHandler;
        using SceneStatus = Engine::Render::Type::SceneStatus;
        using MaskContext = Engine::Render::Type::MaskContext;
        using MaskPairKind = Engine::Render::Type::MaskPairKind;
        using MaskPlan = Engine::Render::Type::MaskPlan;
        using SceneTargetSample = Engine::Render::Type::SceneTargetSample;
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

        // Registers a handler that draws the 3D passes of the overlay at the scene point: the exit of the opaque color pass 0x26E500, the only
        // place they are drawn. At most one draw per frame.
        // note: Runs on the game render thread inside a GpuStateGuard. It returns whether it drew.
        auto OnScene(SceneHandler handler) -> void;

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

        // return: Why the handlers must not run at the scene point, or nothing if they can: no thread of Present, no targets (not ready,
        // resizing or shutting down), a bound target of another device, of another size, with more than one sample, or a viewport 0 that is not
        // the whole target (split screen). The target is the HDR target of the game: it need not be the back buffer but must be a texture of one
        // sample and the size of the back buffer.
        static auto FindSceneBlocker(const SceneTargetSample& sample, bool isReady, std::uint32_t width, std::uint32_t height)
            -> std::optional<SceneStatus>;

        // return: The factor the overlay passes multiply their RGB by at the scene point the slider, or 1 when it is not finite or
        // not positive.
        static auto ComputeSceneColorScale(float slider) -> float;

        // Runs at the exit of each call of the opaque color pass 0x26E500, after the original: the first call of the Present frame on the thread of
        // Present, when nothing blocks it, runs the scene handlers inside a GpuStateGuard, with the HDR target bound in slot 0 and the depth stencil
        // of the store. What they draw is inside the frame of the game: the transparents, effects, blade and HUD of the game are drawn over it.
        // Without a call (the hook is not installed, a menu, a pause without a draw) nothing is drawn in the frame.
        // note: Called by the first person detour on the game render thread. Nothing escapes: an exception is caught and counted
        // (RenderStore::CountSceneFailure).
        auto OnSceneDraw() -> void;

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
        std::vector<SceneHandler> m_OnScene{};
        bool m_IsSceneLogged{ false };      // thread of Present only
        std::vector<ShutdownCallback> m_OnShutdown{};

        std::mutex m_ShutdownMutex{};
        std::atomic<int> m_HandlerErrors{ 0 };
        bool m_IsAttachFailed{ false };

        // Attaches to the swap chain and raises OnInitialized the first time, OnResize otherwise.
        auto Bind(IDXGISwapChain* swapChain) -> void;

        // Takes the device and creates the targets. Logs the first failure only.
        // return: False if the device or the render target could not be obtained.
        auto Attach(IDXGISwapChain* swapChain) -> bool;

        // param isInFrame: Whether the mask is the one of the game frame in progress (the scene point) or of the last completed
        // one (the events at Present).
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

        // The body of OnSceneDraw. Exceptions are caught by the caller.
        auto HandleSceneDraw() -> void;

        // Runs the scene handlers inside a GpuStateGuard.
        // return: Drawn, Failed (a handler threw) or Declined (none drew).
        auto RunSceneHandlers(const FrameContext& frame, ID3D11DeviceContext* context) -> SceneStatus;

        // Reads the render target bound in slot 0 and the viewport into a sample. Stores nothing.
        auto MeasureBoundTarget() -> SceneTargetSample;

        // return: The immediate context when the call runs on the thread of Present with the store ready, null otherwise.
        auto GetPresentContext() -> ID3D11DeviceContext*;

        auto Raise(const std::vector<FrameHandler>& handlers, const char* stage) -> void;

        // Logs the first 5 handler errors only.
        // note: Never throws.
        auto ReportHandlerError(const char* stage) noexcept -> void;
    };
}