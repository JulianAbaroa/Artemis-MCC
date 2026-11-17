export module Mod.Port.System:TickProducer;

import Engine.Base.Lifecycle.State;
import Engine.Capture.Type;
import Engine.Capture.System;
import Bridge.Transport.System;
import Bridge.Codec.Pipeline.Type;
import std;

export namespace Mod::Port::System
{
    // Gives the tick hooks the copy of every completed tick (D305): reads the clock of the bridge, copies the game with CaptureService (full
    // or light, with the same rules the AI thread had in sparse mode, Q101) and queues the result for the AI thread.
    // note: Game thread only, so it keeps its state without a lock. It queues only while the AI thread consumes (the queue is open) and has
    // loaded the resources of the current teardown epoch (LifecycleStore). Otherwise the tick goes by as it did with no reader.
    class TickProducerAdapter : public Engine::Capture::Type::TickProducer
    {
    private:
        using LifecycleStore = Engine::Base::Lifecycle::State::LifecycleStore;
        using CaptureService = Engine::Capture::System::CaptureService;
        using ProducedTick = Engine::Capture::Type::ProducedTick;
        using BridgeService = Bridge::Transport::System::BridgeService;
        using TickQueue = Bridge::Codec::Pipeline::Type::TickQueue;

    public:
        TickProducerAdapter(LifecycleStore& lifecycleStore, CaptureService& captureService, BridgeService& bridgeService,
            TickQueue& tickQueue) :
            m_LifecycleStore(lifecycleStore), m_CaptureService(captureService), m_BridgeService(bridgeService), m_TickQueue(tickQueue) {}
        ~TickProducerAdapter() override = default;

        TickProducerAdapter(const TickProducerAdapter&) = delete;
        auto operator=(const TickProducerAdapter&) -> TickProducerAdapter& = delete;

        // Reads the clock, copies the tick and queues it. The queue wait is capped by k_ReadTimeout: past it the tick is lost, counted in
        // the next queued one (DroppedBefore) and the clock does not count the jump as a skip. Neither does it count the ticks that went by
        // while the queue was shut or the map was not loaded.
        // note: The first tick of a teardown epoch starts the numbering and the sparse state over. A decision tick that was queued is marked
        // for the bridge, so Lockstep waits for its answer however far behind the AI thread is. Marked means queued for the AI thread, not
        // delivered to the client.
        auto Produce() -> ProducedTick override;

    private:
        LifecycleStore& m_LifecycleStore;
        CaptureService& m_CaptureService;
        BridgeService& m_BridgeService;
        TickQueue& m_TickQueue;

        // Teardown epoch the state below belongs to, or the maximum before the first tick.
        std::uint64_t m_Epoch{ std::numeric_limits<std::uint64_t>::max() };
        // Epoch whose first tick after the load was already queued.
        std::uint64_t m_FirstDoneEpoch{ std::numeric_limits<std::uint64_t>::max() };
        // Ticks produced since the map loaded.
        std::uint64_t m_Generation{ 0 };
        // Ticks lost since the last one that was queued.
        std::uint64_t m_Lost{ 0 };
        // Whether the previous tick that was queued was in sparse mode (Q101).
        bool m_IsPreviousSparse{ false };
        // Whether ticks went by without a clock read since the last one (the queue was shut or the map was not loaded): the next read is not
        // judged as consecutive, so the jump is not counted as a clock skip.
        bool m_IsClockStale{ false };
    };
}