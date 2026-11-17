module Mod.Port.System;
import :TickProducer;

import Control.Lockstep.System;
import Bridge.Codec.Flat.Type;
import std;

namespace
{
    using Bridge::Codec::Flat::Type::BridgeClock;
    using Bridge::Codec::Pipeline::Type::CapturedTick;
    using Control::Lockstep::System::IsLightTick;
    using Engine::Capture::Type::k_ReadTimeout;

    using std::chrono::nanoseconds;
    using std::chrono::steady_clock;

    auto ElapsedNs(steady_clock::time_point start, steady_clock::time_point end) -> std::uint64_t
    {
        return static_cast<std::uint64_t>(std::chrono::duration_cast<nanoseconds>(end - start).count());
    }
}

namespace Mod::Port::System
{
    auto TickProducerAdapter::Produce() -> ProducedTick
    {
        const std::uint64_t epoch{ m_LifecycleStore.GetTeardownEpoch() };
        if (epoch != m_Epoch)
        {
            m_Epoch = epoch;
            m_Generation = 0;
            m_Lost = 0;
            m_IsPreviousSparse = false;
            m_IsClockStale = false;
            m_BridgeService.ResetClock();
        }

        // The loaded epoch is the one the AI thread loaded the resources under: after a teardown it never matches until the next load, so
        // nothing is queued for the old map and no tick of the new one before its resources are there.
        // The clock is not read on a tick that goes by, so the next read comes after ticks nobody saw: it must not count that as a skip.
        if (!m_TickQueue.IsOpen() || m_LifecycleStore.GetLoadedEpoch() != epoch || !m_LifecycleStore.CanPublish())
        {
            m_IsClockStale = true;
            return {};
        }

        const auto captureStart{ steady_clock::now() };

        BridgeClock clock{};
        m_BridgeService.CaptureClock(m_Lost == 0 && !m_IsClockStale, clock);
        m_IsClockStale = false;

        const bool isFirstAfterLoad{ m_FirstDoneEpoch != epoch };
        const bool isLight{ IsLightTick(clock.IsSparse, clock.IsDecision, m_IsPreviousSparse, isFirstAfterLoad) };

        CapturedTick tick{
            .Snapshot = isLight ? m_CaptureService.CaptureLight() : m_CaptureService.Capture(),
            .Clock = clock,
            .Generation = ++m_Generation,
            .Epoch = epoch,
            .IsLight = isLight,
            .DroppedBefore = m_Lost,
            .IsFirstAfterLoad = isFirstAfterLoad };
        const auto pushStart{ steady_clock::now() };
        ProducedTick produced{ .CaptureNs = ElapsedNs(captureStart, pushStart) };

        const auto result{ m_TickQueue.Push(std::move(tick), k_ReadTimeout) };
        produced.QueueWaitNs = ElapsedNs(pushStart, steady_clock::now());
        produced.QueueDepth = result.Depth;

        if (result.IsQueued)
        {
            // Only a tick the AI thread will see moves the state forward: after a lost one the next tick is still the first after the load, or
            // the first after sparse turned on, so it stays a full capture.
            if (!isLight) m_FirstDoneEpoch = epoch;
            m_IsPreviousSparse = clock.IsSparse;
            m_Lost = 0;
            if (clock.IsDecision) m_BridgeService.MarkPushed(clock.ActionTick);
        }
        else if (result.IsTimedOut)
        {
            ++m_Lost;
            produced.IsLost = true;
        }
        else
        {
            // The queue closed while the tick was being copied: nobody consumes it, so it is not a lost tick.
            --m_Generation;
        }

        return produced;
    }
}