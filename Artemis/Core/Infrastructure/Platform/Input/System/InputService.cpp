module Platform.Input.System;

using namespace std::chrono_literals;

namespace
{
    using Context = Platform::Input::Type::Context;
    using Action = Platform::Input::Type::Action;
    using Clock = std::chrono::steady_clock;
}

namespace Platform::Input::System
{
    auto InputService::AutomaticInput() -> void
    {
        Request currentReq = { Context::Unknown, Action::Unknown };

        if (!m_InputStore.DequeueRequest(currentReq) ||
            currentReq.Action == Action::Unknown) return;

        // switch (currentReq.Action)
        // {
        // default:
        //     auto condition = []() { return false; };
        //     this->InjectInput(currentReq, condition, 100ms, 50ms);
        //     break;
        // }
    }

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

    auto InputService::ReportHandlerError(const char* stage) -> void
    {
        if (m_HandlerErrors.fetch_add(1) >= 5) return;

        m_LogsService.Message("[InputService] ERROR:"
            " A {} handler threw an exception", stage);
    }

    auto InputService::InjectInput(Request request, 
        std::function<bool()> successCondition,
        Milliseconds timeoutMs, Milliseconds stabilizeMs) -> bool
    {
        m_InputStore.SetNextRequest(request.Context, request.Action);

        auto startWait = Clock::now();
        bool success = false;

        while (Clock::now() - startWait < timeoutMs)
        {
            if (successCondition())
            {
                success = true;
                break;
            }

            std::this_thread::yield();
        }

        m_InputStore.SetNextRequest(Context::Theater, Action::Unknown);

        if (stabilizeMs > 0ms) std::this_thread::sleep_for(stabilizeMs);

        m_InputStore.SetProcessing(false);

        return success;
    }
}