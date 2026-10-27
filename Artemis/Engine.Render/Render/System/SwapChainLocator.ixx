module;

#include <d3d11.h>

export module Platform.Render.System:SwapChainLocator;

import Common.Log.System;
import Platform.Render.Type;
import std;

export namespace Platform::Render::System
{
    // Finds the addresses of the IDXGISwapChain methods without touching the game swap chain.
    class SwapChainLocator
    {
    private:
        using Log = Common::Log::System::Log;

        using SwapChainAddresses = Platform::Render::Type::SwapChainAddresses;

    public:
        explicit SwapChainLocator(Log& log) :
            m_Log(log) {}
        ~SwapChainLocator() = default;

        SwapChainLocator(const SwapChainLocator&) = delete;
        auto operator=(const SwapChainLocator&) -> SwapChainLocator& = delete;

        // Creates a dummy window, device and swap chain and reads the methods from the vtable.
        // return: The addresses, cached after the first success. Empty if it fails.
        auto Locate() -> SwapChainAddresses;

    private:
        Log& m_Log;

        SwapChainAddresses m_Addresses{};
    };
}