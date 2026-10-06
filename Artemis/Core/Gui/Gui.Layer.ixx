export module Gui.Layer;

import Service.Layer;
import Platform.Layer;
import Platform.Render.Type;
import Gui.Backend.System;
import std;

export namespace Gui
{
    // Root of the Gui layer. Owns the ImGui backend and the window procedure handler.
    // note: The backend follows the render events of the Platform layer, so it starts, resizes, draws and shuts down with the game render.
    class Layer
    {
    private:
        using FrameContext = Platform::Render::Type::FrameContext;

        using BackendGuiService = Gui::Backend::System::BackendGuiService;
        using WndProcGuiService = Gui::Backend::System::WndProcGuiService;

    public:
        Layer(Service::Layer& service, Platform::Layer& platform) :
            m_BackendGuiService(service.m_LogsService, service.m_SettingsStore, platform.m_RenderStore),
            m_WndProcGuiService(m_BackendGuiService, service.m_SettingsStore)
        {
            platform.m_RenderService.OnInitialized([this](const FrameContext& frame) {
                m_BackendGuiService.Initialize(frame);
            });

            platform.m_RenderService.OnResize([this](const FrameContext& frame) {
                m_BackendGuiService.OnResize(frame);
            });

            platform.m_RenderService.OnFrame([this](const FrameContext&) {
                m_BackendGuiService.NewFrame();
            });

            platform.m_RenderService.OnPostFrame([this](const FrameContext&) {
                m_BackendGuiService.Render();
            });

            platform.m_RenderService.OnShutdown([this] {
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