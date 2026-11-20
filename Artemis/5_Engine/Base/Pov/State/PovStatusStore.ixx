export module Engine.Base.Pov.State:Status;

import Common.Engine.Type;
import Engine.Base.Pov.Type;
import std;

namespace
{
    using Common::Engine::Type::k_NoHandle;
}

export namespace Engine::Base::Pov::State
{
    // What the POV did. The game thread writes it at every update and reads back its own last update to decide the next one (ownership of the
    // view and the start of a request). Other threads only display it, or wait on AssignedHandle at the unload.
    // note: Atomics read one by one, so a read from another thread can mix two updates: no other thread decides anything from it.
    class PovStatusStore
    {
    private:
        using PovStatus = Engine::Base::Pov::Type::PovStatus;

    public:
        PovStatusStore() = default;
        ~PovStatusStore() = default;

        PovStatusStore(const PovStatusStore&) = delete;
        auto operator=(const PovStatusStore&) -> PovStatusStore& = delete;

        // Publishes the four handles of an update (none is 0xFFFFFFFF).
        auto SetHandles(std::uint32_t requested, std::uint32_t engine, std::uint32_t local, std::uint32_t assigned) -> void;

        // Counts a call of the engine function. A reassert is one made after the engine changed the view on its own.
        auto CountCall(bool isReassert) -> void;

        // Counts a fault caught around the engine function.
        auto CountFault() -> void;

        // Zeroes the calls and the reasserts: a request began.
        auto ResetCounts() -> void;

        auto SetInstalled(bool isInstalled) -> void;

        // Zeroes everything, the installed flag included: a new engine starts clean.
        auto Reset() -> void;

        auto Read() const -> PovStatus;

    private:
        std::atomic<std::uint32_t> m_Requested{ k_NoHandle };
        std::atomic<std::uint32_t> m_Engine{ k_NoHandle };
        std::atomic<std::uint32_t> m_Local{ k_NoHandle };
        std::atomic<std::uint32_t> m_Assigned{ k_NoHandle };
        std::atomic<std::uint32_t> m_Calls{ 0 };
        std::atomic<std::uint32_t> m_Reasserts{ 0 };
        std::atomic<std::uint32_t> m_Faults{ 0 };
        std::atomic<bool> m_IsInstalled{ false };
    };
}