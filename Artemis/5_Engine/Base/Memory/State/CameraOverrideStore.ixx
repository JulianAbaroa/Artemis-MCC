export module Engine.Base.Memory.State:CameraOverride;

import Engine.Base.Memory.Type;
import std;

export namespace Engine::Base::Memory::State
{
    // The pose the game camera must take, or none. One thread writes it (the render thread of the Viewer) and the game thread reads it.
    // note: Double buffered: the writer fills the buffer that is not current and then makes it current, so a read finds a complete pose even when
    // the writer is preempted in the middle of a write. Each buffer also has a sequence, so a reader that the writer laps twice retries on the
    // newer one. Exactly one writer thread (checked in debug builds). Lock free and without allocations on both sides.
    class CameraOverrideStore
    {
    private:
        using CameraOverride = Engine::Base::Memory::Type::ObserverCamera::CameraOverride;
        using CameraOverrideSlot = Engine::Base::Memory::Type::ObserverCamera::CameraOverrideSlot;

    public:
        CameraOverrideStore() = default;
        ~CameraOverrideStore() = default;

        CameraOverrideStore(const CameraOverrideStore&) = delete;
        auto operator=(const CameraOverrideStore&) -> CameraOverrideStore& = delete;

        // Replaces the pose, marks it active and stamps it with the steady clock.
        // note: The pose is not validated here.
        auto Publish(const CameraOverride& pose) -> void;

        // Marks the pose inactive.
        auto Clear() -> void;

        // param maxAge: A pose published longer ago than this is taken as none.
        // return: The pose, or nothing when it is inactive or older than maxAge.
        // note: Wait free and without allocations, for the game thread.
        auto Read(std::chrono::nanoseconds maxAge) const -> std::optional<CameraOverride>;

        // return: Whether a pose is published (not cleared), however old.
        auto IsActive() const -> bool;

        // Counts a fault the hook caught while it read or wrote the engine memory.
        // note: Any thread. The hook cannot log on the game thread, so the Viewer shows the count.
        auto CountFault() -> void;

        auto GetFaultCount() const -> std::uint32_t;

    private:
        static constexpr int k_ReadAttempts{ 4 };

        std::array<CameraOverrideSlot, 2> m_Slots{};
        std::atomic<std::uint32_t> m_Current{ 0 };
        std::atomic<std::uint32_t> m_FaultCount{ 0 };

        auto Write(bool isActive, const CameraOverride& pose) -> void;

#ifndef NDEBUG
        std::atomic<std::thread::id> m_Writer{};
#endif
    };
}