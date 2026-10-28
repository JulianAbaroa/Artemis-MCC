export module Gui.Layer;

import Service.Layer;
import Platform.Render.Type;
import Platform.Render.State;
import Platform.Render.System;
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
        using RenderStore = Platform::Render::State::RenderStore;
        using RenderService = Platform::Render::System::RenderService;

        using BackendGuiService = Gui::Backend::System::BackendGuiService;
        using WndProcGuiService = Gui::Backend::System::WndProcGuiService;

    public:
        Layer(Service::Layer& service, RenderService& renderService, RenderStore& renderStore) :
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