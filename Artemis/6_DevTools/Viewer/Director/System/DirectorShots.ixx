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

    // return: Whether the shot can be played: finite numbers, seconds above zero, and for an orbit a radius that is not negative.
    auto IsShotValid(const Director::Type::Shot& shot) -> bool;

    // return: A fixed shot with the pose of the free camera (yaw and pitch in radians).
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

    // return: The index of the shot to go to, or nothing to stay in this one. The shot ends when its seconds pass, or when the round changes with cutting on
    // new rounds on, unless it started less than k_NoDoubleCutSeconds ago. After the last shot comes the first. An index out of the list goes to the first,
    // an empty list does nothing. Seconds that are not above zero count as the default.
    auto Advance(const Director::Type::AdvanceInput& input) -> std::optional<std::size_t>;

    // return: The handle of the next Artemis bot of the roster after the one of lastHandle, in the order of the roster and wrapping, skipping humans and dead
    // bots. Without a last handle in the roster it starts at the beginning. Nothing when there is no live bot.
    auto PickPovAgent(std::span<const Roster::Type::RosterEntry> roster, std::uint32_t lastHandle) -> std::optional<std::uint32_t>;

    // return: The first shot that can be played, looking at first and then going by direction (+1 or -1) once around the list. A POV shot can be played
    // only when canPov. Nothing when no shot qualifies, so the caller never loops.
    auto NextPlayableShot(const Director::Type::ShotList& shots, std::size_t first, int direction, bool canPov) -> std::optional<std::size_t>;

    // return: The player the POV request must carry this frame, or nothing to withdraw it. While the director plays the request is its own: the player of a
    // POV shot, and nothing for any other shot, whatever Watch POV and the selection say. Otherwise Watch POV with an observed player.
    // param directedPlayer: The player of the POV shot that plays, k_NoHandle for another kind of shot or a wait.
    // param observed: The player observed (the selected one), k_NoHandle when none.
    auto DecidePovRequest(bool isDirectorPlaying, std::uint32_t directedPlayer, bool isWatchPovOn, std::uint32_t observed) -> std::optional<std::uint32_t>;

    // return: The shots of the text. A line is a comment when it starts with '#', empty lines are ignored, and a line that does not parse is skipped and
    // counted. Numbers use '.', yaw and pitch are in degrees.
    //   fixed|<name>|<seconds>|<x> <y> <z>|<yaw> <pitch>
    //   orbit|<name>|<seconds>|<cx> <cy> <cz>|<radius> <height> <start degrees> <degrees per second>
    //   pov|<name>|<seconds>
    auto ParseShots(std::string_view text) -> Director::Type::ParsedShots;

    // return: The text of the list in the format of ParseShots, with a comment header, one shot per line.
    auto FormatShots(const Director::Type::ShotList& shots) -> std::string;

    // return: The map name as a file name: letters, digits, '-', '_' and '.' are kept, every other character becomes '_', up to 64 characters and never
    // starting with a dot. "default" for an empty name.
    auto SanitizeMapKey(std::string_view name) -> std::string;
}