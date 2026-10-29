export module Mod.Thread.System:Dump;

import Services.Layer;
import Engine.Base.Layer;
import DevTools.Workbench.TickDump.System;
import std;

export namespace Mod::Thread::System
{
    // Thread of the TickDump (D151): writes the Ticks the AI thread queues while a recording runs, and ends the recording when the
    // lifecycle stops.
    // note: Without a recording each update only waits on the empty queue.
    class DumpThread
    {
    private:
        using TickDumpUIService = DevTools::Workbench::TickDump::System::TickDumpUIService;

    public:
        DumpThread(Services::Layer& service, Engine::Base::Layer& base, TickDumpUIService& tickDumpUIService) :
            m_Service(service), m_Base(base), m_TickDumpUIService(tickDumpUIService) {}
        ~DumpThread() = default;

        // Blocks until the lifecycle stops.
        auto Run() -> void;

    private:
        Services::Layer& m_Service;
        Engine::Base::Layer& m_Base;
        TickDumpUIService& m_TickDumpUIService;
    };
}