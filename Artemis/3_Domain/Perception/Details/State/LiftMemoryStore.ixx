export module Perception.Details.State:LiftMemory;

import Perception.Details.Type;
import std;

export namespace Perception::Details::State
{
    // The LiftMemory of the lift details service between ticks.
    // note: Get runs on the update thread. Cleanup runs on the lifecycle thread, so it only marks the store and the next Get empties it.
    class LiftMemoryStore
    {
    public:
        LiftMemoryStore() = default;
        ~LiftMemoryStore() = default;

        LiftMemoryStore(const LiftMemoryStore&) = delete;
        auto operator=(const LiftMemoryStore&) -> LiftMemoryStore& = delete;

        // return: The memory to read and write, empty if a cleanup marked the store.
        auto Get() -> Perception::Details::Type::LiftMemory&;

        // Empties the memory on the next Get.
        auto Cleanup() -> void;

    private:
        Perception::Details::Type::LiftMemory m_Memory{};
        std::atomic<bool> m_IsStale{ false };
    };
}