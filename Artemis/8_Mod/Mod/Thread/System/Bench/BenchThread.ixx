export module Mod.Thread.System:Bench;

import Services.Layer;
import Engine.Base.Layer;
import DevTools.Workbench.Bench.System;
import std;

export namespace Mod::Thread::System
{
    // Thread of the in-game test bench (D129): turns it on and advances it five times per second while the lifecycle runs.
    // note: When the bench is off, each update returns at once, so the thread only waits for the shutdown.
    class BenchThread
    {
    private:
        using BenchUIService = DevTools::Workbench::Bench::System::BenchUIService;

    public:
        BenchThread(Services::Layer& service, Engine::Base::Layer& base, BenchUIService& benchUIService) :
            m_Service(service), m_Base(base), m_BenchUIService(benchUIService) {}
        ~BenchThread() = default;

        // Blocks until the lifecycle stops.
        auto Run() -> void;

    private:
        Services::Layer& m_Service;
        Engine::Base::Layer& m_Base;
        BenchUIService& m_BenchUIService;
    };
}