module;

#include <d3d11.h>
#include <dxgi.h>

export module Platform.Render.Hook:ResizeBuffers;

import Service.Logs.System;
import Platform.Render.State;
import Platform.Render.System;
import std;

export namespace Platform::Render::Hook
{
	class ResizeBuffersDetour
	{
	private:
		using LogsService = Service::Logs::System::LogsService;
		using RenderStore = Platform::Render::State::RenderStore;
		using RenderService = Platform::Render::System::RenderService;
		using SwapChainLocator = Platform::Render::System::SwapChainLocator;

	public:
		ResizeBuffersDetour(LogsService& logsService, RenderStore& renderStore, 
			RenderService& renderService, SwapChainLocator& swapChainLocator) :
			m_LogsService(logsService), m_RenderStore(renderStore), 
			m_RenderService(renderService), m_SwapChainLocator(swapChainLocator) {}
		~ResizeBuffersDetour() = default;

		auto Install() -> bool;
		auto Uninstall() -> void;

	private:
		LogsService& m_LogsService;
		RenderStore& m_RenderStore;
		RenderService& m_RenderService;
		SwapChainLocator& m_SwapChainLocator;

		static ResizeBuffersDetour* s_Instance;

		static auto __stdcall HookedResizeBuffers(IDXGISwapChain* pSwapChain,
			UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat,
			UINT SwapChainFlags)->HRESULT;

		typedef auto(__stdcall* ResizeBuffers_t)(IDXGISwapChain* pSwapChain,
			UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat,
			UINT SwapChainFlags)->HRESULT;

		static inline ResizeBuffers_t m_OriginalFunction = nullptr;
		std::atomic<void*> m_FunctionAddress{ nullptr };
		std::atomic<bool> m_IsHookInstalled{ false };
	};
}