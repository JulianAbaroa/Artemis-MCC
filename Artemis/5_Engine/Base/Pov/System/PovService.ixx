export module Engine.Base.Pov.System;

import Common.Log.System;
import Engine.Base.Memory.System;
import Engine.Base.Pov.Type;
import Engine.Base.Pov.State;
import std;

namespace
{
    using Engine::Base::Pov::Type::k_PovProbeDrainBatch;
}

export namespace Engine::Base::Pov::System
{
    // Decides what to pass to the engine function that assigns the view of user 0 (VW.9a, D340).
    // param requested: The player asked for, none when there is no request.
    // param engineCurrent: The player the view of user 0 shows now.
    // param local: The player of input user 0, 0xFFFFFFFF when there is none.
    // param isRequestedValid: Whether the requested handle is a player of the table with the same salt.
    // param ownsView: Whether this service set the view (it assigned a player and has not given the view back).
    // return: The handle to assign, or nothing when the view is already right or there is nothing safe to assign. A valid request wins over the
    // local player. No request or an invalid one goes back to the local player only when this service owns the view, so without the Viewer the
    // view is never touched (theater, join in progress). Without a local player nothing is touched.
    auto DecidePovCall(std::optional<std::uint32_t> requested, std::uint32_t engineCurrent, std::uint32_t local, bool isRequestedValid, bool ownsView)
        -> std::optional<std::uint32_t>;

    // Bookkeeping of the ownership of the view after an update.
    // param previous: The player this service had set (0xFFFFFFFF when none).
    // param assignedNow: The player the engine function was called with and accepted in this update, nothing when no call was made or it faulted.
    // param engineCurrent: The player the view showed before this update.
    // param local: The player of input user 0, 0xFFFFFFFF when there is none.
    // return: The player this service holds in the view after the update. When the view shows the local player nothing is the service's, whether it
    // got there by the return call of this update or by itself (the request ended or became invalid with the engine already on local, or the request
    // is the local player). A call that set another player makes it the service's. Otherwise it is unchanged.
    auto NextAssigned(std::uint32_t previous, std::optional<std::uint32_t> assignedNow, std::uint32_t engineCurrent, std::uint32_t local) -> std::uint32_t;

    // return: Whether the status says the view of user 0 shows a requested player that is not the local one.
    auto IsPovOnOtherPlayer(const Engine::Base::Pov::Type::PovStatus& status) -> bool;

    // Makes the view of user 0 show the player the Viewer asks for (VW.9a, D340), with the function of the engine the theater uses, and puts it back
    // on the local player when the request ends.
    // note: Game thread only: it calls an engine function. The engine reassigns the view by itself (map load, players that join or leave), so
    // Update compares at every call and assigns only when the view differs.
    class PovService
    {
    private:
        using Log = Common::Log::System::Log;

        using AOBService = Engine::Base::Memory::System::AOBService;
        using MemoryReaderService = Engine::Base::Memory::System::MemoryReaderService;

        using PovAssignFunction = Engine::Base::Pov::Type::PovAssignFunction;
        using PovProbeCursor = Engine::Base::Pov::Type::PovProbeCursor;
        using PovProbeEvent = Engine::Base::Pov::Type::PovProbeEvent;

        using PovProbeStore = Engine::Base::Pov::State::PovProbeStore;
        using PovRequestStore = Engine::Base::Pov::State::PovRequestStore;
        using PovStatusStore = Engine::Base::Pov::State::PovStatusStore;

    public:
        PovService(Log& log, AOBService& aobService, MemoryReaderService& reader, PovRequestStore& requestStore, PovStatusStore& statusStore,
            PovProbeStore& probeStore) :
            m_Log(log), m_AOBService(aobService), m_Reader(reader), m_RequestStore(requestStore), m_StatusStore(statusStore),
            m_ProbeStore(probeStore) {}
        ~PovService() = default;

        PovService(const PovService&) = delete;
        auto operator=(const PovService&) -> PovService& = delete;

        // Finds the engine function by its RVA and prologue. Not found is logged as an ERROR once and Update does nothing. Does nothing if found.
        // return: True if the function is found.
        // note: The function is code of the game module, loaded again for every match: find it with the engine, forget it at the unhook.
        auto Install() -> bool;

        // Forgets the function and clears the installed flag. Does nothing if it was not found.
        auto Uninstall() -> void;

        // Gives the view back to the local player at the unload, whatever thread it runs on (B2).
        // param timeout: Longest wait for the game thread when this is not the game thread.
        // note: It first makes Update ignore the request. On the game thread (the thread of the last Update) it puts the view of user 0 back on the
        // player of input user 0 at once, if this service set it, with the thread context of that Update, and returns without waiting. On another
        // thread (the unload runs on the main thread) it waits until Update has put the view back (AssignedHandle none) or the timeout passes
        // (WARNING, and it goes on). Call it before the hook of SimulationTicksDetour is removed, or nothing is left to serve the return. It does
        // nothing when the service owns no view.
        auto Release(std::chrono::milliseconds timeout) -> void;

        // Reads the request and the view of user 0 and assigns the player that DecidePovCall gives, if any. Publishes the status.
        // param threadContext: The engine thread context of the game thread, 0 if there is none.
        // note: Game thread only, before the ticks of the frame. Does nothing without the function or without a readable player mapping.
        // It ends with the probe of VW.9a.x (TEMPORAL): while a POV is requested it writes to the log what the probe hooks noted and the state of
        // the views when it changes ("[PovProbe]" lines). With no request it writes nothing. It never changes the game.
        auto Update(std::uintptr_t threadContext) -> void;

    private:
        Log& m_Log;
        AOBService& m_AOBService;
        MemoryReaderService& m_Reader;
        PovRequestStore& m_RequestStore;
        PovStatusStore& m_StatusStore;
        PovProbeStore& m_ProbeStore;

        std::atomic<PovAssignFunction> m_Assign{ nullptr };

        // Set by Release: Update ignores the request.
        std::atomic<bool> m_IsReleased{ false };

        // The thread and the thread context of the last Update, which is how Release knows the game thread.
        std::atomic<std::thread::id> m_GameThread{};
        std::atomic<std::uintptr_t> m_GameThreadContext{ 0 };

        // Puts the view back on the local player with the thread context of the game thread. Game thread only.
        auto RestoreView(std::uintptr_t threadContext) -> void;

        // The engine function faulted: logs an ERROR once and forgets it, so the service stays off until the next Install.
        auto Disable() -> void;

        // The view logic of Update, without the probe.
        auto UpdateView(std::uintptr_t threadContext) -> void;

        // TEMPORAL VW.9a.x: the probe. Game thread only.
        auto Probe(std::uintptr_t threadContext) -> void;

        // Writes one probe line through the limit of lines per second, counting the ones it drops.
        auto ProbeEmit(const std::string& text) -> void;

        // Writes the two loss counters ("ring lost" and "lines dropped") if they are not zero and starts them over.
        auto ProbeFlushLosses() -> void;

        // Forgets everything the probe kept (active flag, module ranges, last snapshot and cursor, counters), so nothing leaks into the next engine.
        auto ProbeReset() -> void;

        std::array<PovProbeEvent, k_PovProbeDrainBatch> m_ProbeEvents{};
        std::string m_ProbeLastBody{};
        PovProbeCursor m_ProbeLastCursor{};
        std::uint64_t m_ProbeFrame{ 0 };
        std::uint64_t m_ProbeModuleBase{ 0 };
        std::uint64_t m_ProbeModuleSize{ 0 };
        std::uint64_t m_ProbeOwnBase{ 0 };
        std::uint64_t m_ProbeOwnSize{ 0 };
        bool m_IsProbeRunning{ false };
        std::chrono::steady_clock::time_point m_ProbeWindowStart{};
        std::uint32_t m_ProbeLines{ 0 };
        std::uint32_t m_ProbeLinesDropped{ 0 };
        std::uint32_t m_ProbeRingLost{ 0 };
    };
}