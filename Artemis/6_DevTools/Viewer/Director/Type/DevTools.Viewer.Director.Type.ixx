export module DevTools.Viewer.Director.Type;

import Common.Engine.Type;
import Common.Math.Type;
import DevTools.Viewer.Roster.Type;
import std;

namespace
{
    using Common::Engine::Type::k_NoHandle;
    using Common::Math::Type::Vec3;
    using DevTools::Viewer::Roster::Type::RosterEntry;
}

export namespace DevTools::Viewer::Director::Type
{
    // What a shot of the director does with the camera (VW.14). Fixed holds a saved pose, Orbit turns around a point and Pov shows the next Artemis bot in first person.
    enum class ShotKind : std::uint8_t
    {
        Fixed,
        Orbit,
        Pov
    };

    // When a shot ends by itself (VW.16). Seconds cuts only by real time, Rounds at the N-th round change since the shot started and Hold never.
    enum class ShotEndKind : std::uint8_t
    {
        Seconds,
        Rounds,
        Hold
    };

    // Seconds a shot lasts when nothing says otherwise.
    inline constexpr float k_DefaultShotSeconds{ 20.0f };

    // Round changes a shot with an end by rounds waits for when nothing says otherwise.
    inline constexpr float k_DefaultShotRounds{ 1.0f };

    // Degrees per second of a new orbit: one turn per minute.
    inline constexpr float k_DefaultDegreesPerSecond{ 6.0f };

    // A round change that comes less than this many real seconds after a shot started is not counted for its end, because the new round usually arrives
    // right after a cut by the end of the shot before.
    inline constexpr double k_NoDoubleCutSeconds{ 3.0 };

    // Real seconds between two tries to find a playable shot when none is (a list of POV shots without bots).
    inline constexpr double k_RetrySeconds{ 1.0 };

    // Orbit made from a camera that looks at nothing: the center is where the view line meets the plane this far below the eye, or this far ahead of the eye
    // when the camera does not look down. The center is never farther than the maximum distance.
    inline constexpr float k_NoHitDrop{ 5.0f };
    inline constexpr float k_NoHitDistance{ 10.0f };
    inline constexpr float k_MaxCenterDistance{ 100.0f };

    // The end of a shot. Value is the seconds of Seconds (above zero) and the round changes of Rounds (a whole number, one or more). Hold has no value.
    struct ShotEnd
    {
        ShotEndKind Kind{ ShotEndKind::Seconds };
        float Value{ k_DefaultShotSeconds };
    };

    // One shot of the library. Fields of the other kinds are ignored.
    struct Shot
    {
        // Unique in the file. It is given when the shot is created (the store, never reused in the session) and the sequences refer to it.
        std::uint32_t Id{ 0 };
        ShotKind Kind{ ShotKind::Fixed };
        std::string Name{};

        // A shot that is not enabled stays in the library and in the sequences, but is not played.
        bool IsEnabled{ true };
        ShotEnd End{};

        // Fixed: the pose of the free camera. Yaw and pitch are in radians, as CameraService has them.
        Vec3 Eye{};
        float Yaw{ 0.0f };
        float Pitch{ 0.0f };

        // Orbit: the camera turns around Center at Radius (horizontal) and Height above it (eye.z - center.z), always looking at Center.
        // StartDegrees is the angle around Z at the start and DegreesPerSecond the turn speed (negative turns the other way).
        Vec3 Center{};
        float Radius{ 0.0f };
        float Height{ 0.0f };
        float StartDegrees{ 0.0f };
        float DegreesPerSecond{ k_DefaultDegreesPerSecond };
    };

    // The shots of one map, in library order.
    struct ShotList : std::vector<Shot>
    {
        using vector::vector;
    };

    // Ids of shots, in the order they play. An id may repeat.
    struct ShotIdList : std::vector<std::uint32_t>
    {
        using vector::vector;
    };

    // Positions in the library of the shots to play, in the order they play.
    struct ShotIndexList : std::vector<std::size_t>
    {
        using vector::vector;
    };

    // A named order of shots of the library.
    struct Sequence
    {
        std::string Name{};
        ShotIdList ShotIds{};
    };

    struct SequenceList : std::vector<Sequence>
    {
        using vector::vector;
    };

    // The file of a map: the library, the sequences and the active one (empty for none: the whole library plays).
    struct DirectorFile
    {
        ShotList Shots{};
        SequenceList Sequences{};
        std::string ActiveSequence{};
    };

    // The file read from text, how many lines were skipped for being invalid, how many ids of sequences named no shot and whether the text was of the
    // first version (so it was migrated and is written as the current one at the next save).
    struct ParsedDirector
    {
        DirectorFile File{};
        std::size_t Skipped{ 0 };
        std::size_t DroppedIds{ 0 };
        bool IsMigrated{ false };
    };

    // Eye and view direction of a shot at one moment.
    struct ShotPose
    {
        Vec3 Eye{};
        Vec3 Forward{ 1.0f, 0.0f, 0.0f };
    };

    // The yaw and pitch of a direction, in radians.
    struct ShotYawPitch
    {
        float Yaw{ 0.0f };
        float Pitch{ 0.0f };
    };

    // What identifies a round: the match (an engine start) and the round of the match. A change of key is a new round, also a new match.
    struct RoundKey
    {
        std::uint64_t MatchId{ 0 };
        std::int16_t Round{ 0 };

        friend auto operator==(const RoundKey&, const RoundKey&) -> bool = default;
    };

    // What the decision to leave a shot looks at. Index and Count are positions in the playlist.
    struct AdvanceInput
    {
        std::size_t Count{ 0 };
        std::size_t Index{ 0 };

        // Real seconds since the shot started.
        double ElapsedSeconds{ 0.0 };
        ShotEnd End{};

        // Round changes since the shot started, without the ones of its first k_NoDoubleCutSeconds.
        std::uint32_t RoundChanges{ 0 };
    };

    // What the director sees in one frame.
    struct DirectorInput
    {
        std::chrono::steady_clock::time_point Now{};

        // Whether there is a Tick, so the roster is known. Without it no POV shot starts and none ends.
        bool HasRoster{ false };
        std::span<const RosterEntry> Roster{};

        // Round of the match of the Tick, none without a Match.
        std::optional<std::int16_t> Round{};
    };

    // What the director asks of the camera in one frame. HasShot is false while it plays and waits for a shot it can show: the camera stays as it is.
    struct DirectorFrame
    {
        bool IsPlaying{ false };
        bool HasShot{ false };
        ShotKind Kind{ ShotKind::Fixed };

        // Fixed and Orbit: the pose the free camera takes.
        Vec3 Eye{};
        float Yaw{ 0.0f };
        float Pitch{ 0.0f };

        // Pov: the player to show, k_NoHandle for the other kinds.
        std::uint32_t PovPlayer{ k_NoHandle };
    };

    // Where playback is, for the panel and the telemetry. Index and Count are positions in the playlist (the sequence without the shots that are off).
    struct DirectorStatus
    {
        bool IsPlaying{ false };
        bool IsHolding{ false };
        std::size_t Index{ 0 };
        std::size_t Count{ 0 };
        float ElapsedSeconds{ 0.0f };
        std::uint32_t RoundChanges{ 0 };
        std::uint32_t ShotId{ 0 };
        ShotKind Kind{ ShotKind::Fixed };
        ShotEnd End{};
        std::string ShotName{};

        // The sequence that plays, empty when the whole library does.
        std::string SequenceName{};
        std::uint32_t PovPlayer{ k_NoHandle };
    };

    // The file of the map: the key, the path (empty without the Artemis folder) and what the last load or save said.
    struct DirectorFileInfo
    {
        std::string MapKey{};
        std::string Path{};
        std::string Message{};
    };
}