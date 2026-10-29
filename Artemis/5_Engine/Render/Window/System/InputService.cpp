module Engine.Render.Window.System;
import :Input;

namespace Engine::Render::Window::System
{
    auto InputService::OnWindowMessage(WindowMessageHandler handler, bool runFirst) -> void
    {
        if (runFirst)
        {
            m_OnWindowMessage.insert(m_OnWindowMessage.begin(), std::move(handler));
            return;
        }

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

    auto InputService::ReportHandlerError(const char* stage) -> void
    {
        if (m_HandlerErrors.fetch_add(1) >= 5) return;

        m_Log.Message("[InputService] ERROR:"
            " A {} handler threw an exception", stage);
    }
}