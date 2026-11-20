export module Engine.Base.Pov.Type;

import Common.Engine.Type;
import Blam.Layout.Player.Type;
import std;

namespace
{
    using Common::Engine::Type::k_NoHandle;

    using Blam::Layout::Player::Type::Size::k_Base;
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
    // PlayerAssignUnit, the cursor candidate (the update of a UI screen) and the three creators of UI screens.
    inline constexpr std::uint32_t k_PovProbeHookCount{ 8 };

    // Events the ring of the probe holds before the oldest is overwritten. A power of two.
    inline constexpr std::size_t k_PovProbeRingCapacity{ 512 };

    // Same for the ring of the creations of screens.
    inline constexpr std::size_t k_PovProbeCreateRingCapacity{ 64 };

    // Events the game thread copies out of the ring in one batch.
    inline constexpr std::size_t k_PovProbeDrainBatch{ 256 };

    // Distinct this pointers of the cursor candidate noted between two updates.
    inline constexpr std::size_t k_PovProbeMaxThis{ 4 };

    // Safety limit of the probe: lines written to the log per second of the clock. Past it one line says how many were dropped.
    inline constexpr std::uint32_t k_PovProbeMaxLinesPerSecond{ 400 };

    // Words an event carries besides its fixed fields: the return addresses of a screen creation or the fields of a screen.
    inline constexpr std::size_t k_PovProbeExtraWords{ 12 };

    // Words of one slot of the ring: meta, caller, four arguments, the result and the extra words.
    inline constexpr std::size_t k_PovProbeSlotWords{ 1 + 1 + 4 + 1 + k_PovProbeExtraWords };

    // Fields of a UI screen the probe follows (PovProbeScreenField), screens followed at once, ticks without an update after which a screen is gone.
    inline constexpr std::size_t k_PovProbeScreenFieldCount{ 9 };
    inline constexpr std::size_t k_PovProbeScreenSlots{ 16 };
    inline constexpr std::uint32_t k_PovProbeScreenMaxAge{ 60 };

    // Different stacks of screen creation the dump remembers (the first time a stack shows up it is written in full) per activation.
    inline constexpr std::size_t k_PovProbeStackTable{ 64 };

    // Controllers whose state the snapshot reads, and players followed by the player diff (the view and the input user 0).
    inline constexpr std::uint32_t k_PovProbeControllers{ 4 };
    inline constexpr std::size_t k_PovProbePlayerRoles{ 2 };

    // Most changed words a pdiff line lists.
    inline constexpr std::size_t k_PovProbeDiffWords{ 16 };

    // Size of a player datum the diff compares, in uint32 words (the datum is Blam.Layout.Player k_Base bytes).
    inline constexpr std::size_t k_PovProbePlayerWords{ k_Base / sizeof(std::uint32_t) };

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

        // 0x2C9EFC, 0x2C9F88 and 0x2CA014: creators of a UI screen. Args are window, name, a4 and a5, Result the screen, Extra the return addresses.
        ScreenCreate,
        ScreenCreateAlt,
        ScreenCreateEx,

        // A screen showed up in the update 0x2D2018 or one of its fields changed. Args are this and the mask of the changed fields (bit per
        // PovProbeScreenField, all of them for a new one), Extra the fields.
        ScreenNew,
        ScreenChange,

        // Same as ScreenChange, seen in the sample taken when the update returned.
        ScreenChangePost,

        // The table of screens was full and a screen left it. Args are the screen that left and the evictions so far.
        ScreenEvict,
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

        // The value the function returned, when it was noted after the call.
        std::uint64_t Result{};

        // How many of the Extra words are used.
        std::uint32_t ExtraCount{};
        std::array<std::uint64_t, k_PovProbeExtraWords> Extra{};
    };

    // One slot of the ring: a ticket (the number of the event plus one, zero while a writer fills it) and the words of the event.
    struct PovProbeSlot
    {
        std::atomic<std::uint64_t> Ticket{ 0 };
        std::array<std::atomic<std::uint64_t>, k_PovProbeSlotWords> Words{};
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

    // The fields of a UI screen the probe follows, in the order of PovProbeScreen::Values.
    enum class PovProbeScreenField : std::uint8_t
    {
        Vtable,
        Window,
        Mask,
        Flags,
        State,
        Field1AC,
        Field154,
        Field1C0,
        Field1C4,
    };

    // The fields of a UI screen as read at the start of one update (a signed field is sign extended).
    struct PovProbeScreen
    {
        std::array<std::uint64_t, k_PovProbeScreenFieldCount> Values{};
    };

    // One followed screen. Only the game thread writes it in practice, every word is atomic so another thread cannot tear it.
    struct PovProbeScreenSlot
    {
        // Zero when the slot is free.
        std::atomic<std::uint64_t> This{ 0 };
        std::atomic<std::uint64_t> Birth{ 0 };
        std::atomic<std::uint32_t> FirstTick{ 0 };
        std::atomic<std::uint32_t> LastTick{ 0 };
        std::atomic<std::uint32_t> Calls{ 0 };
        std::array<std::atomic<std::uint64_t>, k_PovProbeScreenFieldCount> Values{};
    };

    // A screen whose update was not called for k_PovProbeScreenMaxAge ticks.
    struct PovProbeGoneScreen
    {
        std::uint64_t This{};
        std::uint32_t FirstTick{};
        std::uint32_t LastTick{};
        std::uint32_t Calls{};
    };

    // Why the probe is on.
    enum class PovProbeReason : std::uint8_t
    {
        None,

        // A POV is requested.
        Pov,

        // The player of the view of user 0 has no unit.
        ViewDead,

        // The player of the input user 0 has no unit.
        LocalDead,
    };

    // The respawn state of one player, as the probe reads it.
    struct PovProbePlayerState
    {
        std::uint32_t Handle{ k_NoHandle };
        std::optional<std::uint32_t> Unit{};
        std::optional<std::uint16_t> RespawnTicks{};
        std::optional<std::uint16_t> RespawnElapsed{};
        std::optional<std::uint16_t> RespawnSeconds{};
    };

    // The last copy of the datum of a player, for the diff between two ticks.
    struct PovProbePlayerCopy
    {
        bool IsValid{};
        std::uint32_t Handle{ k_NoHandle };
        std::array<std::uint32_t, k_PovProbePlayerWords> Words{};
    };
}