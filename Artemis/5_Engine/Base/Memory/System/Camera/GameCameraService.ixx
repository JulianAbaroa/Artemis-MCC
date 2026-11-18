export module Engine.Base.Memory.System:Camera;

import :Reader;

import Engine.Base.Memory.Type;
import std;

namespace
{
    using Engine::Base::Memory::Type::GameCamera::GameCamera;

    using Engine::Base::Memory::Type::GameCamera::k_ProjectionFloatCount;
}

export namespace Engine::Base::Memory::System
{
    // Reads the camera the game rendered the last view with (s_render_projection, a global of haloreach.dll). Read only.
    class GameCameraService
    {
    public:
        explicit GameCameraService(MemoryReaderService& reader) : m_Reader(reader) {}
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

    private:
        MemoryReaderService& m_Reader;
    };
}