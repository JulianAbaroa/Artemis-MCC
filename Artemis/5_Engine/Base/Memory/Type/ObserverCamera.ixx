export module Engine.Base.Memory.Type:ObserverCamera;

import Common.Math.Type;
import std;

export namespace Engine::Base::Memory::Type::ObserverCamera
{
    // The observer globals of the game thread (A21 sec. 2 and 6, VW.5b): *(thread context + k_ThreadContextObserverGlobalsOffset), 0x1050 bytes,
    // one block of k_ObserverUserStride bytes per local user.
    inline constexpr std::uintptr_t k_ThreadContextObserverGlobalsOffset{ 0x688 };
    inline constexpr std::size_t k_ObserverUserStride{ 0x410 };

    // Offsets inside the block of a user. The WORK is what the director asks for (already smoothed) and what the engine turns into the camera
    // position, the location (culling) and the vertical field of view when it builds the result.
    // note: Position = focus + offset - forward * distance. The hook writes exactly these three ranges and puts back exactly these bytes: focus and
    // offset (0x1C8, 0x18 bytes), distance (0x1E8, 4 bytes) and forward then up (0x1F0, 0x18 bytes). The bytes between them (0x1E0 to 0x1E7) and
    // the field of view (0x1EC) are never touched: the engine writes the field of view itself during the call.
    inline constexpr std::size_t k_ObserverWorkFocus{ 0x1C8 };
    inline constexpr std::size_t k_ObserverWorkFocusSize{ 0x18 };
    inline constexpr std::size_t k_ObserverWorkDistance{ 0x1E8 };
    inline constexpr std::size_t k_ObserverWorkDistanceSize{ 0x4 };
    inline constexpr std::size_t k_ObserverWorkAxes{ 0x1F0 };
    inline constexpr std::size_t k_ObserverWorkAxesSize{ 0x18 };

    // Byte that makes the engine apply the matrix at +0x208 (a camera relative to a parent, such as a vehicle). It is zeroed during the call so
    // the eye stays in world space.
    inline constexpr std::size_t k_ObserverParentFlag{ 0x11E };

    // The user whose camera is driven (the first local player).
    inline constexpr std::uint32_t k_OverrideUser{ 0 };

    // A requested pose is valid only if its eye is nearer than this to the origin (the engine clamps positions to +-50000).
    inline constexpr float k_OverrideMaxEyeLength{ 50000.0f };

    // Largest error of the length of an axis, and of the dot of the two axes, for a requested pose to count as orthonormal.
    inline constexpr float k_OverrideTolerance{ 1e-3f };

    // A pose older than this is ignored by the hook. The Viewer asks for it every frame, so an older one means nobody is driving it any more (a
    // map load, a stalled Present, the next engine) and the camera must go back to the player instead of staying where it was left.
    inline constexpr std::chrono::milliseconds k_OverrideMaxAge{ 100 };

    // The pose the game camera is asked to take.
    struct CameraOverride
    {
        Common::Math::Type::Vec3 Eye{};
        Common::Math::Type::Vec3 Forward{};
        Common::Math::Type::Vec3 Up{};
    };

    // One of the two buffers of the CameraOverrideStore: a pose, whether it is active and when it was published, behind its own sequence.
    struct CameraOverrideSlot
    {
        std::atomic<std::uint32_t> Sequence{ 0 };
        std::atomic<bool> IsActive{ false };
        std::atomic<std::int64_t> StampNanoseconds{ 0 };
        std::array<std::atomic<float>, 9> Values{};
    };

    // Signature of the engine function that builds the camera result of a user: void(uint32 user, void* out).
    using ObserverCameraFunction = auto(__fastcall*)(std::uint32_t, void*) -> void;
}