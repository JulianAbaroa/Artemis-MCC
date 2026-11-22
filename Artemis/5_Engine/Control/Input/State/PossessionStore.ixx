export module Engine.Control.Input.State:Possession;

import Engine.Control.Input.Type;
import std;

export namespace Engine::Control::Input::State
{
    // The client state of the bot the user possesses, and the counters of the panel.
    // note: One possession at a time. The bot is touched by the game thread only (the 0x5df80 hook once per frame and the PlayerAction hook once per
    // tick). The status is atomic: the UI reads it.
    class PossessionStore
    {
    private:
        using PossessedBot = Engine::Control::Input::Type::PossessedBot;
        using PossessStatus = Engine::Control::Input::Type::PossessStatus;

    public:
        PossessionStore() = default;
        ~PossessionStore() = default;

        PossessionStore(const PossessionStore&) = delete;
        auto operator=(const PossessionStore&) -> PossessionStore& = delete;

        // Starts the possession of a bot with a new state, or goes on with the one it has when it is the same bot (same handle).
        // return: True when it started a new possession (the state is new), false when it goes on.
        // note: Game thread only. Any other possessed bot is released (one possession at a time).
        auto Begin(std::size_t playerIndex, std::uint32_t handle) -> bool;

        // Drops the state of the bot. Game thread only. Does nothing when none is possessed.
        auto Release() -> void;

        // return: Whether the calling thread is the one that began the possession (the thread of the ticks). The bot is shared by the thread of the
        // ticks and the one of the frames without a lock, which is only right if they are the same thread.
        auto IsTickThread() const -> bool;

        // return: The thread that began the possession (the thread of the ticks), a default id when none.
        auto GetTickThread() const -> std::thread::id;

        // Turns the possession off for good (until the next load of the mod) because the frames ran on another thread than the ticks.
        // return: True the first time (the caller logs one ERROR).
        // note: Any thread. After it no frame touches the bot and the next tick releases it.
        auto Disable() -> bool;

        auto IsDisabled() const -> bool;

        // return: True the first time in this possession (the caller logs the threads once), false after. Game thread only.
        auto MarkThreadsLogged() -> bool;

        // return: The possessed bot, or null when none. Game thread only.
        auto Find() -> PossessedBot*;

        // return: Whether a bot is possessed. Any thread.
        auto IsActive() const -> bool;

        // Counts one possessed tick. Game thread only.
        // param isDelivered: The action came from the client of the user.
        // param isUnsupported: The client left a branch it does not port.
        auto CountTick(bool isDelivered, bool isUnsupported) -> void;

        // Marks that the unit of the possessed bot is missing.
        // return: True the first time in this possession (the caller logs one WARNING), false after.
        auto MarkUnitMissing() -> bool;

        // return: The counters. Any thread.
        auto GetStatus() const -> PossessStatus;

    private:
        std::optional<PossessedBot> m_Bot{};
        std::atomic<std::thread::id> m_TickThread{};
        bool m_IsThreadLogDone{ false };
        std::atomic<bool> m_IsDisabled{ false };

        std::atomic<std::uint32_t> m_Player{ 0xFFFFFFFF };
        std::atomic<std::uint64_t> m_Ticks{ 0 };
        std::atomic<std::uint64_t> m_Delivered{ 0 };
        std::atomic<std::uint64_t> m_Unsupported{ 0 };
        std::atomic<bool> m_IsUnitMissing{ false };
    };
}