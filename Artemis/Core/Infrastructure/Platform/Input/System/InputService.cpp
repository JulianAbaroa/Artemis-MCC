module;

#include <windows.h>

module Platform.Input.System;

namespace Platform::Input::System
{
    auto InputService::OnWindowMessage(WindowMessageHandler handler) -> void
    {
        m_OnWindowMessage.push_back(std::move(handler));
    }

    auto InputService::OnRawMouse(RawMouseHandler handler) -> void
    {
        m_OnRawMouse.push_back(std::move(handler));
    }

    auto InputService::RaiseWindowMessage(WindowMessage& message) -> bool
    {
        for (auto& handler : m_OnWindowMessage)
        {
            try
            {
                if (handler(message)) return true;
            }
            catch (...)
            {
                this->ReportHandlerError("OnWindowMessage");
            }
        }

        return false;
    }

    auto InputService::RaiseRawMouse(RawMouse& mouse) -> bool
    {
        for (auto& handler : m_OnRawMouse)
        {
            try
            {
                if (handler(mouse)) return true;
            }
            catch (...)
            {
                this->ReportHandlerError("OnRawMouse");
            }
        }

        return false;
    }

    auto InputService::BindWindow(HWND window) -> void
    {
        m_Window.store(window);
    }

    auto InputService::SetAIControlActive(bool active) -> void
    {
        m_MouseDeltaStore.SetAIControlActive(active);
    }

    auto InputService::InjectMouseDelta(std::int32_t deltaX, std::int32_t deltaY) -> bool
    {
        if (!m_MouseDeltaStore.IsAIControlActive()) return false;

        const HWND window = m_Window.load();
        if (!window) return false;

        m_MouseDeltaStore.AddPendingDelta(deltaX, deltaY);

        const auto k_SyntheticHandle = reinterpret_cast<HRAWINPUT>(
            static_cast<std::uintptr_t>(0xA1A1A1A1));

        return PostMessageW(window, WM_INPUT, MAKEWPARAM(RIM_INPUT, 0),
            reinterpret_cast<LPARAM>(k_SyntheticHandle)) != FALSE;
    }

    auto InputService::SetActionRequested(Action action, bool requested) -> void
    {
        m_InputStore.SetActionRequested(action, requested);
    }

    auto InputService::AdvanceInputTick() -> void
    {
        m_InputStore.AdvanceTick();
    }

    auto InputService::ReportHandlerError(const char* stage) -> void
    {
        if (m_HandlerErrors.fetch_add(1) >= 5) return;

        m_LogsService.Message("[InputService] ERROR:"
            " A {} handler threw an exception", stage);
    }
}