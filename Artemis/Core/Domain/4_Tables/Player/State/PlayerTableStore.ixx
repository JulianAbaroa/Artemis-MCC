export module Tables.Player.State;

import Tables.Player.Type;
import std;

export namespace Tables::Player::State
{
    // Holds the base address of the engine player table and the players read from it.
    // note: Writers lock a mutex. Readers use the published table and never wait.
    class PlayerTableStore
    {
    private:
        using AlivePlayer = Tables::Player::Type::Alive::AlivePlayer;
        using PlayerTable = Tables::Player::Type::Alive::PlayerTable;

    public:
        PlayerTableStore() = default;
        ~PlayerTableStore() = default;

        // return: Address of the player table, or 0 if it was not found yet.
        auto GetBase() const -> std::uintptr_t;
        auto SetBase(std::uintptr_t pointer) -> void;

        auto AddPlayer(std::uint32_t handle, const AlivePlayer& player) -> void;
        auto HasPlayer(std::uint32_t handle) const -> bool;
        auto RemovePlayer(std::uint32_t handle) -> void;

        // Runs the processor on every player.
        // param processor: Called with the handle and the player. Runs under the lock.
        template<typename Processor>
        auto UpdatePlayers(Processor&& processor) -> void
        {
            std::lock_guard<std::mutex> lock(m_Mutex);

            for (auto& [handle, player] : m_PlayerTable)
            {
                processor(handle, player);
            }
        }

        // Copies the players into the table that Acquire returns.
        auto Publish() -> void;

        // return: Last published table, or null before the first one and after Cleanup.
        auto Acquire() const -> std::shared_ptr<const PlayerTable>;

        // Clears the players, the base address and the published table.
        auto Cleanup() -> void;

    private:
        std::atomic<std::uintptr_t> m_PlayerTableBase{ 0 };
        std::atomic<std::shared_ptr<const PlayerTable>> m_PublishedPlayerTable{};

        PlayerTable m_PlayerTable{};
        mutable std::mutex m_Mutex{};
    };
}