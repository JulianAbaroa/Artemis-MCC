export module Runtime.Thread:Dump;

import Service.Layer;
import Platform.Layer;
import UI.TickDump.System;
import std;

export namespace Runtime::Thread
{
    // Thread of the TickDump (D151): writes the Ticks the AI thread queues while a recording runs, and ends the recording when the
    // lifecycle stops.
    // note: Without a recording each update only waits on the empty queue.
    class DumpThread
    {
    private:
        using TickDumpUIService = UI::TickDump::System::TickDumpUIService;

    public:
        DumpThread(Service::Layer& service, Platform::Layer& platform, TickDumpUIService& tickDumpUIService) :
            m_Service(service), m_Platform(platform), m_TickDumpUIService(tickDumpUIService) {}
        ~DumpThread() = default;

        // Blocks until the lifecycle stops.
        auto Run() -> void;

    private:
        Service::Layer& m_Service;
        Platform::Layer& m_Platform;
        TickDumpUIService& m_TickDumpUIService;
    };
}