export module Runtime.Thread:Bench;

import Service.Layer;
import Engine.Base.Layer;
import UI.Bench.System;
import std;

export namespace Runtime::Thread
{
    // Thread of the in-game test bench (D129): turns it on and advances it five times per second while the lifecycle runs.
    // note: When the bench is off, each update returns at once, so the thread only waits for the shutdown.
    class BenchThread
    {
    private:
        using BenchUIService = UI::Bench::System::BenchUIService;

    public:
        BenchThread(Service::Layer& service, Engine::Base::Layer& base, BenchUIService& benchUIService) :
            m_Service(service), m_Base(base), m_BenchUIService(benchUIService) {}
        ~BenchThread() = default;

        // Blocks until the lifecycle stops.
        auto Run() -> void;

    private:
        Service::Layer& m_Service;
        Engine::Base::Layer& m_Base;
        BenchUIService& m_BenchUIService;
    };
}