export module DevTools.Viewer.Latency.Type;

import Common.Math.Type;
import Common.State.Type;
import Engine.Base.Memory.Type;
import std;

namespace
{
    using Common::Math::Type::PoseSource;
    using Common::Math::Type::Vec3;
    using Common::State::Type::PickKind;
    using Engine::Base::Memory::Type::FrameClock::FrameClock;
}

export namespace DevTools::Viewer::Latency::Type
{
    // Frames of the sliding window of the measurement (VW.5c).
    inline constexpr std::size_t k_WindowFrames{ 600 };

    // Bins of the histogram of the lag in ticks: 0, 1, 2, 3 and 4 or more.
    inline constexpr std::size_t k_LagBins{ 5 };

    // The game camera counts as moved between two frames when its position changes by more than this (wu).
    inline constexpr float k_CameraMovedDistance{ 1e-4f };

    // The probe of the interpolation only counts the scenes whose two eyes are further apart than this (wu): the local player is moving.
    inline constexpr float k_EyeMovingDistance{ 0.05f };

    // What the overlay saw in one frame, at the moment it drew.
    struct LatencyInput
    {
        FrameClock Clock{};
        // Tick of the scene that is drawn (Match.GameTick of its Tick), none without a Match.
        std::optional<std::uint32_t> SceneTick{};
        // Position of the game camera, none when it is not valid.
        std::optional<Vec3> Camera{};
        // Game mode of the Viewer camera (not the free camera).
        bool IsGameMode{};
        // Eye of the View of the local player in the scene, none without a View.
        std::optional<Vec3> Eye{};
        // How the scene was picked for the tick T - 1 (VW.5d), None when it was not picked for a target (the newest was taken).
        PickKind Pick{};
        // How the poses of the objects were found for the frame (VW.5e): between two ticks, extrapolated, or as the scene shows them.
        PoseSource Interp{};
    };

    // Eye of the local player in the scene of one tick.
    struct EyeAtTick
    {
        bool IsValid{};
        std::uint32_t Tick{};
        Vec3 Eye{};
    };

    // What the measurement remembers from the previous frames.
    struct LatencyMemory
    {
        bool HasFrame{};
        std::uint32_t Tick{};
        std::optional<Vec3> Camera{};
        // The eye of the scene of the last tick, and the eye of the scene of the tick just before it (only when the ticks are consecutive).
        EyeAtTick Current{};
        EyeAtTick Previous{};
    };

    // Distances from the game camera to the eye of the scene (S), the eye of the tick before (S - 1) and the eye interpolated with the tick fraction.
    struct LatencyProbe
    {
        float ToCurrent{};
        float ToPrevious{};
        float ToLerp{};
    };

    // One frame of the window.
    struct LatencySample
    {
        FrameClock Clock{};
        std::optional<std::uint32_t> SceneTick{};
        PickKind Pick{};
        PoseSource Interp{};

        // Lag in ticks: the tick of the game minus the tick of the scene.
        bool HasLag{};
        std::int32_t Lag{};

        // The tick of the game is the one of the previous frame, and the game camera was read in both frames.
        bool IsSameTick{};
        bool HasCameraStep{};
        bool IsCameraMoved{};

        // The probe applies (Game mode, valid camera, local player moving between two consecutive scenes). With the scene up to date it has the
        // distances, with the scene behind the game it is only counted.
        std::optional<LatencyProbe> Probe{};
        bool IsProbeBehind{};
    };

    // Median and 99th percentile of a distance.
    struct Percentiles
    {
        float P50{};
        float P99{};
    };

    // The window summarized for the telemetry.
    struct LatencyReport
    {
        std::size_t Frames{};

        // The last frame of the window, for the clock lines.
        bool HasLast{};
        LatencySample Last{};

        // Frames with a lag by bin, frames with a negative lag, and the largest lag.
        std::array<std::uint32_t, k_LagBins> Lag{};
        std::uint32_t Negative{};
        std::int32_t MaxLag{};

        // Frames with the tick of the previous frame (with the camera read in both) and, of those, the ones where the camera moved.
        std::uint32_t SameTickFrames{};
        std::uint32_t CameraMovedFrames{};

        // Frames of the probe with the scene up to date, frames with the scene behind the game, and the distances of the first.
        std::uint32_t ProbeFrames{};
        std::uint32_t BehindFrames{};
        Percentiles ToCurrent{};
        Percentiles ToPrevious{};
        Percentiles ToLerp{};

        // Frames drawn for a target tick by how the poses were found (VW.5e): blended between two ticks, extrapolated from the last two, or the
        // pose of the scene as is (interpolation off in the game, no next or older scene, or no overlay target).
        std::uint32_t InterpExact{};
        std::uint32_t InterpExtrapolated{};
        std::uint32_t InterpNone{};
    };

}