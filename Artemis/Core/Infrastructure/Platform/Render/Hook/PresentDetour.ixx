module;

#include <d3d11.h>
#include <dxgi.h>

export module Platform.Render.Hook:Present;

import Service.Logs.System;
import Service.Telemetry.State;
import Platform.Render.State;
import Platform.Render.System;
import std;

export namespace Platform::Render::Hook
{
	class PresentDetour
	{
	private:
		using LogsService = Service::Logs::System::LogsService;
		using TelemetryStore = Service::Telemetry::State::TelemetryStore;
		using RenderStore = Platform::Render::State::RenderStore;
		using RenderService = Platform::Render::System::RenderService;
		using SwapChainLocator = Platform::Render::System::SwapChainLocator;

	public:
		PresentDetour(LogsService& logsService, TelemetryStore& telemetryStore, 
			RenderStore& renderStore, RenderService& renderService,
			SwapChainLocator& swapChainLocator) : m_LogsService(logsService),
			m_TelemetryStore(telemetryStore), m_RenderStore(renderStore), 
			m_RenderService(renderService), m_SwapChainLocator(swapChainLocator) {}
		~PresentDetour() = default;

		auto Install() -> bool;
		auto Uninstall() -> void;

	private:
		LogsService& m_LogsService;
		TelemetryStore& m_TelemetryStore;
		RenderStore& m_RenderStore;
		RenderService& m_RenderService;
		SwapChainLocator& m_SwapChainLocator;

		static PresentDetour* s_Instance;

		static auto __stdcall HookedPresent(IDXGISwapChain* pSwapChain,
			UINT SyncInterval, UINT Flags)->HRESULT;

		typedef auto(__stdcall* Present_t)(IDXGISwapChain* pSwapChain,
			UINT SyncInterval, UINT Flags)->HRESULT;

		static inline Present_t m_OriginalFunction = nullptr;
		std::atomic<void*> m_FunctionAddress{ nullptr };
		std::atomic<bool> m_IsHookInstalled{ false };
	};
}