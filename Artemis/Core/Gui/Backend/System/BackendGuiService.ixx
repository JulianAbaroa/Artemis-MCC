export module Gui.Backend.System:Backend;

import Service.Logs.System;
import Service.Settings.State;
import Platform.Input.Type;
import Platform.Render.Type;
import Platform.Render.State;
import std;

export namespace Gui::Backend::System
{
    // Owns the ImGui context and its Win32 and DX11 backends.
    // Starts and draws the ImGui frame, and applies the scale, alpha and menu visibility settings.
    // note: The initialized flag is atomic because the window procedure reads it while the render thread may shut the backend down.
    class BackendGuiService
    {
    private:
        using LogsService = Service::Logs::System::LogsService;
        using SettingsStore = Service::Settings::State::SettingsStore;

        using WindowMessage = Platform::Input::Type::WindowMessage;
        using FrameContext = Platform::Render::Type::FrameContext;
        using RenderStore = Platform::Render::State::RenderStore;

    public:
        BackendGuiService(LogsService& logsService, SettingsStore& settingsStore,
            RenderStore& renderStore) : m_LogsService(logsService),
            m_SettingsStore(settingsStore), m_RenderStore(renderStore) {}
        ~BackendGuiService() = default;

        BackendGuiService(const BackendGuiService&) = delete;
        auto operator=(const BackendGuiService&) -> BackendGuiService& = delete;

        // Creates the ImGui context and the backends for the window and device of the frame.
        // Logs and does nothing if the frame is incomplete or a backend fails to start.
        // note: Shuts the previous context down first if already initialized.
        auto Initialize(const FrameContext& frame) -> void;

        // Recreates the DX11 device objects after the swap chain was resized.
        // note: Initializes instead if the backend is not initialized yet.
        auto OnResize(const FrameContext& frame) -> void;

        // Destroys the backends and the ImGui context. Does nothing if not initialized.
        auto Shutdown() -> void;

        // Applies the settings and starts an ImGui frame. Does nothing if not ready.
        auto NewFrame() -> void;

        // Draws the ImGui data on the back buffer and restores the render targets of the game.
        // Does nothing if not ready.
        auto Render() -> void;

        // Passes a window message to the ImGui Win32 backend.
        // param message: Its Result is set to the value returned by the backend.
        // return: True if ImGui handled the message.
        auto ForwardMessage(WindowMessage& message) -> bool;

        // Tells if ImGui wants a mouse or keyboard message for itself.
        // return: False for any other message or if not initialized.
        auto WantsCapture(std::uint32_t message) const -> bool;

        auto IsInitialized() const -> bool;

        // return: True if initialized and the render store is ready.
        auto IsReady() const -> bool;

    private:
        LogsService& m_LogsService;
        SettingsStore& m_SettingsStore;
        RenderStore& m_RenderStore;

        std::atomic<bool> m_IsInitialized{ false };

        // Scale already applied to the style. Zero forces the next frame to apply it.
        float m_AppliedScale{ 0.0f };

        // Applies the UI scale when it changed, and the menu alpha every frame.
        auto ApplyPreferences() -> void;

        // Shows the cursor and lets ImGui receive the mouse only while the menu is visible.
        auto ApplyInputMode(bool isMenuVisible) -> void;
    };
}