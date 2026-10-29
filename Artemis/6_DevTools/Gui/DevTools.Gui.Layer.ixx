export module DevTools.Gui.Layer;

import Services.Layer;
import Engine.Render.Type;
import Engine.Render.State;
import Engine.Render.System;
import DevTools.Gui.Backend.System;
import std;

export namespace DevTools::Gui
{
    // Root of the Gui layer. Owns the ImGui backend and the window procedure handler.
    // note: The backend follows the render events of Engine::Render::Layer, so it starts, resizes, draws and shuts down with the game render.
    class Layer
    {
    private:
        using FrameContext = Engine::Render::Type::FrameContext;
        using RenderStore = Engine::Render::State::RenderStore;
        using RenderService = Engine::Render::System::RenderService;

        using BackendGuiService = DevTools::Gui::Backend::System::BackendGuiService;
        using WndProcGuiService = DevTools::Gui::Backend::System::WndProcGuiService;

    public:
        Layer(Services::Layer& service, RenderService& renderService, RenderStore& renderStore) :
            m_BackendGuiService(service.m_Log, service.m_SettingsStore, renderStore),
            m_WndProcGuiService(m_BackendGuiService, service.m_SettingsStore)
        {
            renderService.OnInitialized([this](const FrameContext& frame) {
                m_BackendGuiService.Initialize(frame);
            });

            renderService.OnResize([this](const FrameContext& frame) {
                m_BackendGuiService.OnResize(frame);
            });

            renderService.OnFrame([this](const FrameContext&) {
                m_BackendGuiService.NewFrame();
            });

            renderService.OnPostFrame([this](const FrameContext&) {
                m_BackendGuiService.Render();
            });

            renderService.OnShutdown([this] {
                m_BackendGuiService.Shutdown();
            });
        }
        ~Layer() = default;

        // Not copyable. Members hold references to each other.
        Layer(const Layer&) = delete;
        auto operator=(const Layer&) -> Layer& = delete;

        // --- System ---
        BackendGuiService m_BackendGuiService;
        WndProcGuiService m_WndProcGuiService;
    };
}