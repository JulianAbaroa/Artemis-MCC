module;

#include <d3d11.h>

export module Platform.Render.System:SwapChainLocator;

import Service.Logs.System;
import Platform.Render.Type;
import std;

export namespace Platform::Render::System
{
	class SwapChainLocator
	{
	private:
		using SwapChainAddresses = Platform::Render::Type::SwapChainAddresses;

		using LogsService = Service::Logs::System::LogsService;

		static constexpr std::size_t k_PresentIndex = 8;
		static constexpr std::size_t k_ResizeBuffersIndex = 13;

	public:
		explicit SwapChainLocator(LogsService& logsService) : 
			m_LogsService(logsService) {}
		~SwapChainLocator() = default;

		SwapChainLocator(const SwapChainLocator&) = delete;
		SwapChainLocator& operator=(const SwapChainLocator&) = delete;

		auto Locate() -> SwapChainAddresses;

	private:
		LogsService& m_LogsService;

		SwapChainAddresses m_Addresses{};
	};
}