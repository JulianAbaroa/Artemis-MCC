export module Runtime.Thread:Bench;

import Service.Layer;
import Platform.Layer;
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
        BenchThread(Service::Layer& service, Platform::Layer& platform, BenchUIService& benchUIService) :
            m_Service(service), m_Platform(platform), m_BenchUIService(benchUIService) {}
        ~BenchThread() = default;

        // Blocks until the lifecycle stops.
        auto Run() -> void;

    private:
        Service::Layer& m_Service;
        Platform::Layer& m_Platform;
        BenchUIService& m_BenchUIService;
    };
}