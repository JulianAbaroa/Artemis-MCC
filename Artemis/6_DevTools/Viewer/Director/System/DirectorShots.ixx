export module DevTools.Viewer.Director.System:Shots;

import Common.Math.Type;
import DevTools.Viewer.Roster.Type;
import DevTools.Viewer.Director.Type;
import std;

export namespace DevTools::Viewer::Director::System
{
    // return: The word of the kind in the shot file and the telemetry: fixed, orbit or pov.
    auto ShotKindName(Director::Type::ShotKind kind) -> std::string_view;

    // return: The name a new shot of the kind gets: the kind and the number after the ones the list already has of it ("Orbit 3").
    auto DefaultShotName(const Director::Type::ShotList& shots, Director::Type::ShotKind kind) -> std::string;

    // return: Whether the end can be used: seconds above zero, rounds a whole number of one or more, and hold. Finite numbers only.
    auto IsShotEndValid(const Director::Type::ShotEnd& end) -> bool;

    // return: Whether the shot can be played: a valid end, finite numbers and for an orbit a radius that is not negative.
    auto IsShotValid(const Director::Type::Shot& shot) -> bool;

    // return: A fixed shot with the pose of the free camera (yaw and pitch in radians). The id is left for the store.
    auto MakeFixedShot(std::string name, const Common::Math::Type::Vec3& eye, float yaw, float pitch) -> Director::Type::Shot;

    // return: The eye and the direction to the center of an orbit shot after elapsedSeconds of real time, always looking at the center. Time that is not
    // finite counts as zero.
    auto OrbitPose(const Director::Type::Shot& shot, double elapsedSeconds) -> Director::Type::ShotPose;

    // return: The yaw and pitch of a direction, as the free camera has them. A direction that is not usable gives zero for both.
    auto YawPitchOf(const Common::Math::Type::Vec3& forward) -> Director::Type::ShotYawPitch;

    // return: An orbit that starts exactly where the camera is and looks at its center.
    // param hit: Where the center ray of the camera meets the scene. Without it the center is where forward meets the plane k_NoHitDrop below the eye, or
    // k_NoHitDistance ahead when forward does not go down, and never farther than k_MaxCenterDistance.
    // note: The name and the turn speed are left for the caller.
    auto OrbitFromPose(const Common::Math::Type::Vec3& eye, const Common::Math::Type::Vec3& forward, const std::optional<Common::Math::Type::Vec3>& hit)
        -> Director::Type::Shot;

    // return: Whether the key is one of another round than the last one seen. The first key seen is not a change.
    auto IsRoundChange(const std::optional<Director::Type::RoundKey>& last, const Director::Type::RoundKey& now) -> bool;

    // return: The round changes of the shot after this frame: one more when the round changed and the shot started at least k_NoDoubleCutSeconds ago.
    auto AddRoundChange(std::uint32_t counted, bool isRoundChanged, double elapsedSeconds) -> std::uint32_t;

    // return: The playlist position to go to, or nothing to stay in this shot. Seconds ends when its seconds pass (rounds never cut it), Rounds when the
    // counted round changes reach its value (seconds never cut it) and Hold never. After the last shot comes the first. A position out of the playlist goes
    // to the first, an empty playlist does nothing. A value that is not usable counts as the default of its kind.
    auto Advance(const Director::Type::AdvanceInput& input) -> std::optional<std::size_t>;

    // return: The progress of a shot for the telemetry: "12.3/20 s" for seconds, "12.3 s, round 1/2" for rounds and "12.3 s, hold" for hold.
    auto DescribeShotProgress(const Director::Type::ShotEnd& end, double elapsedSeconds, std::uint32_t roundChanges) -> std::string;

    // return: The handle of the next Artemis bot of the roster after the one of lastHandle, in the order of the roster and wrapping, skipping humans and dead
    // bots. Without a last handle in the roster it starts at the beginning. Nothing when there is no live bot.
    auto PickPovAgent(std::span<const Roster::Type::RosterEntry> roster, std::uint32_t lastHandle) -> std::optional<std::uint32_t>;

    // return: The first playlist position that can be played, looking at first and then going by direction (+1 or -1) once around the playlist. A POV shot
    // can be played only when canPov. Nothing when no shot qualifies, so the caller never loops.
    // param playlist: Library positions of the shots to play, from Playlist. A position out of the library is skipped.
    auto NextPlayableShot(const Director::Type::ShotList& shots, const Director::Type::ShotIndexList& playlist, std::size_t first, int direction, bool canPov)
        -> std::optional<std::size_t>;

    // return: The player the POV request must carry this frame, or nothing to withdraw it. While the director plays the request is its own: the player of a
    // POV shot, and nothing for any other shot, whatever Watch POV and the selection say. Otherwise Watch POV with an observed player.
    // param directedPlayer: The player of the POV shot that plays, k_NoHandle for another kind of shot or a wait.
    // param observed: The player observed (the selected one), k_NoHandle when none.
    auto DecidePovRequest(bool isDirectorPlaying, std::uint32_t directedPlayer, bool isWatchPovOn, std::uint32_t observed) -> std::optional<std::uint32_t>;

    // return: The map name as a file name: letters, digits, '-', '_' and '.' are kept, every other character becomes '_', up to 64 characters and never
    // starting with a dot. "default" for an empty name.
    auto SanitizeMapKey(std::string_view name) -> std::string;
}