export module Engine.Base.Pov.State:Request;

import Engine.Base.Memory.Type;
import Engine.Base.Pov.Type;
import std;

export namespace Engine::Base::Pov::State
{
    // The player the game must show in the view of user 0, or none. One thread writes it (the render thread of the Viewer) and the game thread reads it.
    // note: Double buffered like the CameraOverrideStore: the writer fills the buffer that is not current and then makes it current, so a read finds
    // a complete request even when the writer is preempted. Each buffer has a sequence, so a reader that the writer laps retries on the newer one.
    // Exactly one writer thread (checked in debug builds). Lock free and without allocations on both sides.
    class PovRequestStore
    {
    private:
        using PovRequest = Engine::Base::Pov::Type::PovRequest;
        using PovRequestSlot = Engine::Base::Pov::Type::PovRequestSlot;
        using ReaderStallClock = Engine::Base::Memory::Type::Store::ReaderStallClock;

    public:
        PovRequestStore() = default;
        ~PovRequestStore() = default;

        PovRequestStore(const PovRequestStore&) = delete;
        auto operator=(const PovRequestStore&) -> PovRequestStore& = delete;

        // Replaces the request, marks it active and stamps it with the steady clock.
        // note: The handle is not validated here.
        auto Publish(std::uint32_t playerHandle) -> void;

        // Marks the request inactive.
        auto Clear() -> void;

        // param maxAge: A request published longer ago than this is taken as none. The stalls of the readers do not count (D379).
        // return: The request, or nothing when it is inactive or older than maxAge.
        // note: Wait free and without allocations, for the game thread.
        auto Read(std::chrono::nanoseconds maxAge) const -> std::optional<PovRequest>;

    private:
        std::array<PovRequestSlot, 2> m_Slots{};
        std::atomic<std::uint32_t> m_Current{ 0 };
        mutable ReaderStallClock m_ReaderStall{};

        auto Write(bool isActive, std::uint32_t playerHandle) -> void;

#ifndef NDEBUG
        std::atomic<std::thread::id> m_Writer{};
#endif
    };
}