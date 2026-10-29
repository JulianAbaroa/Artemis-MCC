export module Engine.Render.Layer;

import Services.Layer;
import Engine.Base.Layer;
import Engine.Render.State;
import Engine.Render.System;
import Engine.Render.Hook;
import Engine.Render.Window.System;
import Engine.Render.Window.Hook;

export namespace Engine::Render
{
    // The render of the engine: the swap chain, the frame hooks and the window input.
    // note: Owner only: it has no callbacks and installs nothing. The one that composes it connects the lifecycle and installs the hooks.
    class Layer
    {
    private:
        using RenderStore = Engine::Render::State::RenderStore;
        using RenderService = Engine::Render::System::RenderService;
        using SwapChainLocator = Engine::Render::System::SwapChainLocator;
        using PresentDetour = Engine::Render::Hook::PresentDetour;
        using ResizeBuffersDetour = Engine::Render::Hook::ResizeBuffersDetour;

        using InputService = Engine::Render::Window::System::InputService;
        using GetRawInputDataDetour = Engine::Render::Window::Hook::GetRawInputDataDetour;
        using WndProcDetour = Engine::Render::Window::Hook::WndProcDetour;

    public:
        Layer(Services::Layer& service, Engine::Base::Layer& base) :
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