module;

#include <d3d11.h>

export module Platform.Render.System:SwapChainLocator;

import Service.Logs.System;
import Platform.Render.Type;
import std;

export namespace Platform::Render::System
{
    // Finds the addresses of the IDXGISwapChain methods without touching the game swap chain.
    class SwapChainLocator
    {
    private:
        using LogsService = Service::Logs::System::LogsService;

        using SwapChainAddresses = Platform::Render::Type::SwapChainAddresses;

    public:
        explicit SwapChainLocator(LogsService& logsService) :
            m_LogsService(logsService) {}
        ~SwapChainLocator() = default;

        SwapChainLocator(const SwapChainLocator&) = delete;
        auto operator=(const SwapChainLocator&) -> SwapChainLocator& = delete;

        // Creates a dummy window, device and swap chain and reads the methods from the vtable.
        // return: The addresses, cached after the first success. Empty if it fails.
        auto Locate() -> SwapChainAddresses;

    private:
        LogsService& m_LogsService;

        SwapChainAddresses m_Addresses{};
    };
}