export module Perception.Players.State:GroundMemory;

import Perception.Players.Type;
import std;

export namespace Perception::Players::State
{
    // The GroundMemory of the players service between ticks.
    // note: Get runs on the update thread. Cleanup runs on the lifecycle thread, so it only marks the store and the next Get empties it.
    class GroundMemoryStore
    {
    public:
        GroundMemoryStore() = default;
        ~GroundMemoryStore() = default;

        GroundMemoryStore(const GroundMemoryStore&) = delete;
        auto operator=(const GroundMemoryStore&) -> GroundMemoryStore& = delete;

        // return: The memory to read and write, empty if a cleanup marked the store.
        auto Get() -> Perception::Players::Type::GroundMemory&;

        // Empties the memory on the next Get.
        auto Cleanup() -> void;

    private:
        Perception::Players::Type::GroundMemory m_Memory{};
        std::atomic<bool> m_IsStale{ false };
    };
}