module Runtime.Thread;
import :Bridge;

import Platform.Bridge.Type;
import std;

namespace
{
    using Platform::Bridge::Type::BridgeTick;

    using std::chrono::steady_clock;

    using Platform::Bridge::Type::k_ControlInterval;
}

namespace Runtime::Thread
{
    auto BridgeThread::Run() -> void
    {
        auto& lifecycle{ m_Platform.m_LifecycleStore };
        auto& logs{ m_Service.m_LogsService };

        if (!m_Platform.m_BridgeService.Open())
        {
            logs.Message("[BridgeThread] WARNING: The bridge is off for this session.");
            return;
        }

        logs.Message("[BridgeThread] INFO: Started.");
        auto nextControl{ steady_clock::now() };

        while (lifecycle.IsRunning())
        {
            // A failure of the bridge must not take the game down.
            try
            {
                this->Publish();

                const auto now{ steady_clock::now() };
                if (now >= nextControl)
                {
                    m_Platform.m_BridgeService.Update();
                    nextControl = now + k_ControlInterval;
                }
            }
            catch (const std::exception& exception)
            {
                logs.Message("[BridgeThread] ERROR: Bridge update failed ({}).", exception.what());
            }
        }

        m_Platform.m_BridgeService.Close();
        logs.Message("[BridgeThread] INFO: Stopped.");
    }

    auto BridgeThread::Publish() -> void
    {
        const auto request{ m_Export.m_FlatStore.WaitPop(k_ControlInterval) };
        if (!request || !request->Snapshot) return;

        BridgeTick* flat{ m_Platform.m_BridgeService.BeginTick() };
        if (flat == nullptr) return;

        m_Export.m_FlatService.Flatten(*request->Snapshot, *flat);
        m_Platform.m_BridgeService.EndTick(*flat, request->Clock);
    }
}