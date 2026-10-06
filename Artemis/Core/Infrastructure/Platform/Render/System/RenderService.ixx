module;

#include <d3d11.h>
#include <dxgi.h>
#include <wrl/client.h>

export module Platform.Render.System:RenderService;

import Service.Logs.System;
import Platform.Render.Type;
import Platform.Render.State;
import std;

export namespace Platform::Render::System
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

        using LogsService = Service::Logs::System::LogsService;

        using FrameContext = Platform::Render::Type::FrameContext;
        using FrameHandler = Platform::Render::Type::FrameHandler;
        using ShutdownCallback = Platform::Render::Type::ShutdownCallback;

        using RenderStore = Platform::Render::State::RenderStore;

    public:
        RenderService(LogsService& logsService, RenderStore& renderStore) :
            m_LogsService(logsService), m_RenderStore(renderStore) {}
        ~RenderService() = default;

        RenderService(const RenderService&) = delete;
        auto operator=(const RenderService&) -> RenderService& = delete;

        // Handlers are called in registration order.
        auto OnInitialized(FrameHandler handler) -> void;   // first bind to a swap chain
        auto OnFrame(FrameHandler handler) -> void;         // every frame
        auto OnPostFrame(FrameHandler handler) -> void;     // every frame, after OnFrame
        auto OnResize(FrameHandler handler) -> void;        // after the targets are recreated

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

        // Waits for the calls inside the render hooks to finish.
        // return: False if some are still running after the timeout.
        auto WaitForIdle(milliseconds timeout) -> bool;

        // Runs the shutdown callbacks and releases the store. Does nothing if already done.
        auto Shutdown() -> void;

    private:
        LogsService& m_LogsService;
        RenderStore& m_RenderStore;

        std::vector<FrameHandler> m_OnInitialized{};
        std::vector<FrameHandler> m_OnFrame{};
        std::vector<FrameHandler> m_OnPostFrame{};
        std::vector<FrameHandler> m_OnResize{};
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

        auto Raise(const std::vector<FrameHandler>& handlers, const char* stage) -> void;

        // Logs the first 5 handler errors only.
        auto ReportHandlerError(const char* stage) -> void;
    };
}