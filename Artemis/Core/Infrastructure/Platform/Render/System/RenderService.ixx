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
	class RenderService
	{
	private:
		template <typename T>
		using ComPtr = Microsoft::WRL::ComPtr<T>;
		using SteadyClock = std::chrono::steady_clock;
		using MilliSeconds = std::chrono::milliseconds;
		using FrameContext = Platform::Render::Type::FrameContext;

		using LogsService = Service::Logs::System::LogsService;
		using RenderStore = Platform::Render::State::RenderStore;

	public:
		using Callback = std::function<void()>;
		using FrameHandler = std::function<void(const FrameContext&)>;

		RenderService(LogsService& logsService, RenderStore& renderStore) :
			m_LogsService(logsService), m_RenderStore(renderStore) {}
		~RenderService() = default;

		RenderService(const RenderService&) = delete;
		RenderService& operator=(const RenderService&) = delete;

		auto OnInitialized(FrameHandler handler) -> void;
		auto OnFrame(FrameHandler handler) -> void;
		auto OnResize(FrameHandler handler) -> void;
		auto OnShutdown(Callback callback) -> void;

		auto PresentFrame(IDXGISwapChain* swapChain) -> void;
		auto BeginResize() -> void;
		auto EndResize(IDXGISwapChain* swapChain, HRESULT result) -> void;

		auto RequestShutdown() -> void;
		auto WaitForShutdown(MilliSeconds timeout) -> bool;

		auto WaitForIdle(MilliSeconds timeout) -> bool;

		auto Shutdown() -> void;

	private:
		LogsService& m_LogsService;
		RenderStore& m_RenderStore;

		std::vector<FrameHandler> m_OnInitialized{};
		std::vector<FrameHandler> m_OnFrame{};
		std::vector<FrameHandler> m_OnResize{};
		std::vector<Callback> m_OnShutdown{};

		std::mutex m_ShutdownMutex{};
		std::atomic<int> m_HandlerErrors{ 0 };
		bool m_HasAttachFailed{ false };

		auto Bind(IDXGISwapChain* swapChain) -> void;
		auto Attach(IDXGISwapChain* swapChain) -> bool;

		auto MakeContext() const -> FrameContext;

		auto Raise(const std::vector<FrameHandler>& handlers, const char* stage) -> void;
		auto ReportHandlerError(const char* stage) -> void;
	};
}