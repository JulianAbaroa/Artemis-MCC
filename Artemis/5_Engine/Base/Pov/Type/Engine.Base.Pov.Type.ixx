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
}