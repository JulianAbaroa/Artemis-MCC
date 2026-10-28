export module Engine.Render.Layer;

import Service.Layer;
import Engine.Base.Layer;
import Platform.Render.State;
import Platform.Render.System;
import Platform.Render.Hook;
import Platform.Window.System;
import Platform.Window.Hook;

export namespace Engine::Render
{
    // The render of the engine: the swap chain, the frame hooks and the window input.
    // note: Owner only: it has no callbacks and installs nothing. The one that composes it connects the lifecycle and installs the hooks.
    class Layer
    {
    private:
        using RenderStore = Platform::Render::State::RenderStore;
        using RenderService = Platform::Render::System::RenderService;
        using SwapChainLocator = Platform::Render::System::SwapChainLocator;
        using PresentDetour = Platform::Render::Hook::PresentDetour;
        using ResizeBuffersDetour = Platform::Render::Hook::ResizeBuffersDetour;

        using InputService = Platform::Window::System::InputService;
        using GetRawInputDataDetour = Platform::Window::Hook::GetRawInputDataDetour;
        using WndProcDetour = Platform::Window::Hook::WndProcDetour;

    public:
        Layer(Service::Layer& service, Engine::Base::Layer& base) :
            m_InputService(service.m_Log),
            m_RenderService(service.m_Log, m_RenderStore),
            m_SwapChainLocator(service.m_Log),
            m_GetRawInputDataDetour(service.m_Log, service.m_SettingsStore, m_InputService),
            m_WndProcDetour(service.m_Log, m_InputService, base.m_LifecycleStore, base.m_LifecycleService),
            m_PresentDetour(service.m_Log, service.m_TelemetryStore, m_RenderStore, m_RenderService, m_SwapChainLocator),
            m_ResizeBuffersDetour(service.m_Log, m_RenderStore, m_RenderService, m_SwapChainLocator) {}
        ~Layer() = default;

        // Not copyable. Members hold references to each other.
        Layer(const Layer&) = delete;
        auto operator=(const Layer&) -> Layer& = delete;

        // --- State ---
        RenderStore m_RenderStore{};

        // --- System ---
        InputService m_InputService;
        RenderService m_RenderService;
        SwapChainLocator m_SwapChainLocator;

        // --- Hook ---
        GetRawInputDataDetour m_GetRawInputDataDetour;
        WndProcDetour m_WndProcDetour;
        PresentDetour m_PresentDetour;
        ResizeBuffersDetour m_ResizeBuffersDetour;
    };
}