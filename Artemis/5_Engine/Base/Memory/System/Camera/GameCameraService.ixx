export module Engine.Base.Memory.System:Camera;

import :Reader;

import Common.Math.Type;
import Engine.Base.Memory.Type;
import Engine.Base.Memory.State;
import std;

namespace
{
    using Common::Math::Type::Vec3;
    using Engine::Base::Memory::State::CameraOverrideStore;
    using Engine::Base::Memory::Type::GameCamera::GameCamera;
    using Engine::Base::Memory::Type::ObserverCamera::CameraOverride;

    using Engine::Base::Memory::Type::GameCamera::k_ProjectionFloatCount;
}

export namespace Engine::Base::Memory::System
{
    // Reads the camera the game rendered the last view with (s_render_projection, a global of haloreach.dll) and holds the pose the game camera
    // must take (VW.5b: the observer detour of Engine.Base.Memory.Hook writes it into the engine).
    class GameCameraService
    {
    public:
        GameCameraService(MemoryReaderService& reader, CameraOverrideStore& overrideStore) :
            m_Reader(reader), m_OverrideStore(overrideStore) {}
        ~GameCameraService() = default;

        GameCameraService(const GameCameraService&) = delete;
        auto operator=(const GameCameraService&) -> GameCameraService& = delete;

        // Copies the floats of s_render_projection and validates them.
        // return: The camera, or nothing when the module is not loaded, the read fails or the camera is not valid.
        // note: A plain memory read, no hook and no TLS. The module base is looked up on every call because the game module is loaded again
        // for every match. Meant for the render thread (Present).
        auto Read() -> std::optional<GameCamera>;

        // return: The camera of the floats, or nothing when a used float is not finite, the axes are not orthonormal to k_AxisTolerance or
        // M00 or M11 is not positive.
        static auto Parse(const std::array<float, k_ProjectionFloatCount>& floats) -> std::optional<GameCamera>;

        // Asks the game camera to take this pose from the next frame of the game.
        // return: False when the pose is not valid (see IsValidOverride). The override is then cleared, so the game camera goes back to the player.
        // note: Any thread, but one writer thread at a time (the render thread of the Viewer). It never blocks the game thread.
        auto SetOverride(const Vec3& eye, const Vec3& forward, const Vec3& up) -> bool;

        // The game camera goes back to what the engine computes.
        auto ClearOverride() -> void;

        // return: Whether an override is published.
        auto IsOverrideActive() const -> bool;

        // return: How many faults the observer hook caught while it touched the engine memory (it cannot log on the game thread).
        auto GetOverrideFaultCount() const -> std::uint32_t;

        // return: True when the eye and the axes are finite, forward and up are unit length and perpendicular within k_OverrideTolerance and
        // the eye is nearer than k_OverrideMaxEyeLength to the origin.
        static auto IsValidOverride(const CameraOverride& pose) -> bool;

    private:
        MemoryReaderService& m_Reader;
        CameraOverrideStore& m_OverrideStore;
    };
}