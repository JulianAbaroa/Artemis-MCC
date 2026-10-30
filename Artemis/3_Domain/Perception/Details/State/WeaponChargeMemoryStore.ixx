export module Perception.Details.State:ChargeMemory;

import Perception.Details.Type;
import std;

export namespace Perception::Details::State
{
    // The WeaponChargeMemory of the weapon details service between ticks.
    // note: Get runs on the update thread. Cleanup runs on the lifecycle thread, so it only marks the store and the next Get empties it.
    class WeaponChargeMemoryStore
    {
    public:
        WeaponChargeMemoryStore() = default;
        ~WeaponChargeMemoryStore() = default;

        WeaponChargeMemoryStore(const WeaponChargeMemoryStore&) = delete;
        auto operator=(const WeaponChargeMemoryStore&) -> WeaponChargeMemoryStore& = delete;

        // return: The memory to read and write, empty if a cleanup marked the store.
        auto Get() -> Perception::Details::Type::WeaponChargeMemory&;

        // Empties the memory on the next Get.
        auto Cleanup() -> void;

    private:
        Perception::Details::Type::WeaponChargeMemory m_Memory{};
        std::atomic<bool> m_IsStale{ false };
    };
}