module Runtime.Thread;
import :Dump;

import std;

namespace
{
    using std::chrono::milliseconds;

    // Longest wait for a queued Tick, so requests of the UI and the end of the lifecycle are seen soon.
    constexpr milliseconds k_PopTimeout{ 50 };
}

namespace Runtime::Thread
{
    auto DumpThread::Run() -> void
    {
        auto& lifecycle{ m_Platform.m_LifecycleStore };

        m_Service.m_Log.Message("[DumpThread] INFO: Started.");

        while (lifecycle.IsRunning())
        {
            // A failure of the dump (a full disk, a file it cannot write) must not take the game down.
            try
            {
                m_TickDumpUIService.Update(k_PopTimeout);
            }
            catch (const std::exception& exception)
            {
                m_Service.m_Log.Message("[DumpThread] ERROR: Dump update failed ({}).", exception.what());
            }
        }

        m_TickDumpUIService.Shutdown();
        m_Service.m_Log.Message("[DumpThread] INFO: Stopped.");
    }
}