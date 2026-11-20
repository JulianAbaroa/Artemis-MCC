export module Engine.Base.Pov.Type;

import Common.Engine.Type;
import std;

namespace
{
    using Common::Engine::Type::k_NoHandle;
}

export namespace Engine::Base::Pov::Type
{
    // The first local player (VW.9a, D340): the output user (view) whose player the POV changes and the input user whose player is the local one.
    inline constexpr std::uint32_t k_PovUser{ 0 };

    // A request older than this is no request. The Viewer asks for it at every Present, so an older one means nobody asks any more (the Viewer
    // was turned off, a stalled Present, a map load) and the view must go back to the local player.
    inline constexpr std::chrono::milliseconds k_PovRequestMaxAge{ 100 };

    // Time between two looks of Release at the status while it waits for the game thread.
    inline constexpr std::chrono::milliseconds k_PovReleasePoll{ 5 };

    // The engine function that makes an output user (view) show a player: void(uint32 outputUser, uint32 playerHandle). It is called, not hooked.
    using PovAssignFunction = auto(__fastcall*)(std::uint32_t, std::uint32_t) -> void;

    // The player the game must show in the view of user 0.
    struct PovRequest
    {
        std::uint32_t PlayerHandle{ k_NoHandle };
    };

    // One of the two buffers of the PovRequestStore: a request, whether it is active and when it was published, behind its own sequence.
    struct PovRequestSlot
    {
        std::atomic<std::uint32_t> Sequence{ 0 };
        std::atomic<bool> IsActive{ false };
        std::atomic<std::int64_t> StampNanoseconds{ 0 };
        std::atomic<std::uint32_t> PlayerHandle{ k_NoHandle };
    };

    // What the POV did, for the telemetry. Every handle is k_NoHandle when it is none.
    // note: The fields are read one by one, so a read can mix two updates.
    struct PovStatus
    {
        // The player asked for, as the last update read it.
        std::uint32_t RequestedHandle{ k_NoHandle };

        // The player of the view of user 0, as the last update read it (before its own call).
        std::uint32_t EngineHandle{ k_NoHandle };

        // The player of input user 0.
        std::uint32_t LocalHandle{ k_NoHandle };

        // The player the last call of the service set, none after a return to the local player.
        std::uint32_t AssignedHandle{ k_NoHandle };

        // Calls of the engine function since the request began.
        std::uint32_t Calls{};

        // Calls made to show the requested player again after the engine changed the view on its own, since the request began.
        std::uint32_t Reasserts{};

        // Faults caught around the engine function since it was found (Install starts the count over).
        std::uint32_t Faults{};

        // Whether the engine function was found.
        bool IsInstalled{};
    };

    // TEMPORAL VW.9a.x (PovProbe, deleted when VW.9a.x closes). Hooks of the probe: the view assignment, the unit of a view, the unit of a player,
    // PlayerAssignUnit and the cursor candidate.
    inline constexpr std::uint32_t k_PovProbeHookCount{ 5 };

    // Events the ring of the probe holds before the oldest is overwritten. A power of two.
    inline constexpr std::size_t k_PovProbeRingCapacity{ 512 };

    // Events the game thread copies out of the ring in one batch.
    inline constexpr std::size_t k_PovProbeDrainBatch{ 256 };

    // Distinct this pointers of the cursor candidate noted between two updates.
    inline constexpr std::size_t k_PovProbeMaxThis{ 4 };

    // Safety limit of the probe: lines written to the log per second of the clock. Past it one line says how many were dropped.
    inline constexpr std::uint32_t k_PovProbeMaxLinesPerSecond{ 200 };

    // Views (output users) the snapshot of the probe reads.
    inline constexpr std::uint32_t k_PovProbeViews{ 4 };

    // The value of a field that could not be read.
    inline constexpr std::uint64_t k_PovProbeUnreadable{ 0xFFFFFFFFFFFFFFFF };

    // The probe notes the first four integer arguments of the hooked functions. Its five detours take eight integer arguments and return one, so
    // each forwards to the original exactly what the game passed.
    using PovProbeWideFunction = auto(__fastcall*)(std::uint64_t, std::uint64_t, std::uint64_t, std::uint64_t, std::uint64_t, std::uint64_t,
        std::uint64_t, std::uint64_t) -> std::uint64_t;

    // Which hooked function a noted call belongs to.
    enum class PovProbeKind : std::uint8_t
    {
        // 0x53D7C: void(view, player).
        ViewPlayer,

        // 0x54070: void(view, unit).
        ViewUnit,

        // 0x53CA4: void(player, unit).
        PlayerUnit,

        // 0x45814: PlayerAssignUnit, signature not confirmed (the four arguments are noted raw).
        PlayerAssignUnit,
    };

    // One noted call.
    struct PovProbeEvent
    {
        PovProbeKind Kind{};
        std::uint32_t ThreadId{};

        // The return address of the hooked function.
        std::uint64_t Caller{};

        // The first four integer arguments (rcx, rdx, r8, r9).
        std::array<std::uint64_t, 4> Args{};
    };

    // One slot of the ring: a ticket (the number of the event plus one, zero while a writer fills it) and the words of the event.
    struct PovProbeSlot
    {
        std::atomic<std::uint64_t> Ticket{ 0 };
        std::array<std::atomic<std::uint64_t>, 6> Words{};
    };

    // What one read of the ring gave.
    struct PovProbeDrain
    {
        std::size_t Count{};
        std::uint32_t Dropped{};
    };

    // The cursor candidate since the last take: calls, the first distinct this pointers and the value the last call passed.
    struct PovProbeCursor
    {
        std::uint32_t Calls{};
        std::uint32_t ThisCount{};
        std::array<std::uint64_t, k_PovProbeMaxThis> This{};
        std::uint64_t FirstValue{};
    };
}