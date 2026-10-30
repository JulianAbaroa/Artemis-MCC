export module Perception.Events.State:Memory;

import Perception.Events.Type;
import std;

export namespace Perception::Events::State
{
    // The EventsMemory of the events service between ticks.
    // note: Get runs on the update thread. Cleanup runs on the lifecycle thread, so it only marks the store and the next Get empties it.
    class EventsMemoryStore
    {
    public:
        EventsMemoryStore() = default;
        ~EventsMemoryStore() = default;

        EventsMemoryStore(const EventsMemoryStore&) = delete;
        auto operator=(const EventsMemoryStore&) -> EventsMemoryStore& = delete;

        // return: The memory to read and write, empty if a cleanup marked the store.
        auto Get() -> Perception::Events::Type::EventsMemory&;

        // Empties the memory on the next Get.
        auto Cleanup() -> void;

    private:
        Perception::Events::Type::EventsMemory m_Memory{};
        std::atomic<bool> m_IsStale{ false };
    };
}