export module Engine.Capture.Oracle.System;

import Common.Log.System;
import Engine.Base.Memory.System;
import Engine.Base.Lifecycle.State;
import Engine.Capture.State;
import Engine.Capture.Oracle.Type;
import Engine.Capture.Oracle.State;
import Blam.Layout.Collision.Type;
import std;

export namespace Engine::Capture::Oracle::System
{
    // Traces the rays of the raycast oracle with the engine function the bullets use, so the dev tools can compare them with the rays of
    // Artemis and measure what a ray costs the engine (R3, D272).
    // note: Game thread only, at a tick boundary: ExecuteGameFrameDetour calls Run right after the AI thread finished reading the tick, when
    // the game thread is between two ticks and runs nothing of the engine. The engine function reads the game state and writes the
    // scratch of the thread context of the caller, which is the one of the game thread, so no pool slot is shared with another thread.
    class OracleService
    {
    private:
        using Log = Common::Log::System::Log;

        using AOBService = Engine::Base::Memory::System::AOBService;
        using LifecycleStore = Engine::Base::Lifecycle::State::LifecycleStore;
        using CaptureStore = Engine::Capture::State::CaptureStore;
        using OracleStore = Engine::Capture::Oracle::State::OracleStore;
        using CollisionTestVectorFunction = Engine::Capture::Oracle::Type::CollisionTestVectorFunction;
        using OracleProbe = Engine::Capture::Oracle::Type::OracleProbe;
        using OracleRay = Engine::Capture::Oracle::Type::OracleRay;

        using CollisionResult = Blam::Layout::Collision::Type::Result::CollisionResult;

    public:
        OracleService(Log& log, AOBService& aobService, LifecycleStore& lifecycleStore, CaptureStore& captureStore,
            OracleStore& oracleStore) :
            m_Log(log), m_AOBService(aobService), m_LifecycleStore(lifecycleStore), m_CaptureStore(captureStore),
            m_OracleStore(oracleStore) {}
        ~OracleService() = default;

        OracleService(const OracleService&) = delete;
        auto operator=(const OracleService&) -> OracleService& = delete;

        // Counts the tick and traces the pending batch of rays, if the oracle is on and the engine is running.
        // note: Costs one atomic read when the oracle is off. The engine function is found the first time a batch arrives. A fault inside it
        // turns the oracle off with an ERROR instead of reaching the game.
        auto Run() -> void;

    private:
        Log& m_Log;
        AOBService& m_AOBService;
        LifecycleStore& m_LifecycleStore;
        CaptureStore& m_CaptureStore;
        OracleStore& m_OracleStore;

        CollisionTestVectorFunction m_CollisionTestVector{ nullptr };
        bool m_IsResolveTried{ false };

        // Each message of a skipped Run or of the first batch is logged once.
        bool m_IsSkipLogged{ false };
        bool m_IsFirstOutcomeLogged{ false };

        // QueryPerformanceCounter ticks in a second. Set with the function.
        std::int64_t m_TicksPerSecond{ 0 };

        // Finds the engine function once. Logs the cause and marks the store unavailable if it is not there.
        auto ResolveFunction() -> bool;

        // return: The two answers of the engine for a ray, or nullopt if the engine function faulted.
        auto Probe(const OracleRay& ray) -> std::optional<OracleProbe>;

        // Calls the engine function once.
        // param ignoreHandle: First object to ignore.
        // param result: The decoded answer.
        // return: Nanoseconds the call took, or nullopt if it faulted.
        auto Trace(std::uint64_t flags, const OracleRay& ray, std::uint32_t ignoreHandle, CollisionResult& result)
            -> std::optional<std::uint32_t>;
    };
}