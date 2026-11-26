module DevTools.Viewer.Director.System;
import :Shots;

import Common.Engine.Type;
import Common.Math.Type;
import Common.Math.System;
import DevTools.Viewer.Roster.Type;
import DevTools.Viewer.Director.Type;
import std;

namespace
{
    namespace VecMath = Common::Math::System;

    using Common::Math::Type::Vec3;
    using DevTools::Viewer::Director::Type::AdvanceInput;
    using DevTools::Viewer::Director::Type::RoundKey;
    using DevTools::Viewer::Director::Type::Shot;
    using DevTools::Viewer::Director::Type::ShotKind;
    using DevTools::Viewer::Director::Type::ShotList;
    using DevTools::Viewer::Director::Type::ShotEnd;
    using DevTools::Viewer::Director::Type::ShotEndKind;
    using DevTools::Viewer::Director::Type::ShotIndexList;
    using DevTools::Viewer::Director::Type::ShotPose;
    using DevTools::Viewer::Director::Type::ShotYawPitch;
    using DevTools::Viewer::Roster::Type::RosterEntry;

    using Common::Engine::Type::k_NoHandle;
    using Common::Math::Type::k_DegreesPerRadian;
    using Common::Math::Type::k_RadiansPerDegree;
    using DevTools::Viewer::Director::Type::k_DefaultShotRounds;
    using DevTools::Viewer::Director::Type::k_DefaultShotSeconds;
    using DevTools::Viewer::Director::Type::k_MaxCenterDistance;
    using DevTools::Viewer::Director::Type::k_NoDoubleCutSeconds;
    using DevTools::Viewer::Director::Type::k_NoHitDistance;
    using DevTools::Viewer::Director::Type::k_NoHitDrop;

    constexpr std::size_t k_MaxKeyLength{ 64 };
}

namespace DevTools::Viewer::Director::System
{
    auto ShotKindName(ShotKind kind) -> std::string_view
    {
        switch (kind)
        {
        case ShotKind::Fixed: return "fixed";
        case ShotKind::Orbit: return "orbit";
        case ShotKind::Pov: return "pov";
        }

        return "?";
    }

    auto DefaultShotName(const ShotList& shots, ShotKind kind) -> std::string
    {
        const auto sameKind{ std::ranges::count(shots, kind, &Shot::Kind) };
        const std::string_view label{ kind == ShotKind::Fixed ? "Fixed" : kind == ShotKind::Orbit ? "Orbit" : "POV" };
        return std::format("{} {}", label, sameKind + 1);
    }

    auto IsShotEndValid(const ShotEnd& end) -> bool
    {
        switch (end.Kind)
        {
        case ShotEndKind::Seconds:
            return end.Value > 0.0f && std::isfinite(end.Value);

        case ShotEndKind::Rounds:
            return end.Value >= 1.0f && std::isfinite(end.Value) && end.Value == std::floor(end.Value);

        case ShotEndKind::Hold:
            return true;
        }

        return false;
    }

    auto IsShotValid(const Shot& shot) -> bool
    {
        if (!IsShotEndValid(shot.End)) return false;

        switch (shot.Kind)
        {
        case ShotKind::Fixed:
            return VecMath::IsFinite(shot.Eye) && std::isfinite(shot.Yaw) && std::isfinite(shot.Pitch);

        case ShotKind::Orbit:
            return VecMath::IsFinite(shot.Center) && std::isfinite(shot.Radius) && shot.Radius >= 0.0f && std::isfinite(shot.Height) &&
                std::isfinite(shot.StartDegrees) && std::isfinite(shot.DegreesPerSecond);

        case ShotKind::Pov:
            return true;
        }

        return false;
    }

    auto MakeFixedShot(std::string name, const Vec3& eye, float yaw, float pitch) -> Shot
    {
        Shot shot{};
        shot.Kind = ShotKind::Fixed;
        shot.Name = std::move(name);
        shot.Eye = eye;
        shot.Yaw = yaw;
        shot.Pitch = pitch;
        return shot;
    }

    auto OrbitPose(const Shot& shot, double elapsedSeconds) -> ShotPose
    {
        const double seconds{ std::isfinite(elapsedSeconds) ? elapsedSeconds : 0.0 };

        // The remainder keeps the angle small, so a long run does not lose the precision of the float.
        const double degrees{ std::fmod(static_cast<double>(shot.StartDegrees) + static_cast<double>(shot.DegreesPerSecond) * seconds, 360.0) };
        const float angle{ static_cast<float>(degrees) * k_RadiansPerDegree };

        const Vec3 eye{ shot.Center.X + shot.Radius * std::cos(angle), shot.Center.Y + shot.Radius * std::sin(angle), shot.Center.Z + shot.Height };

        return ShotPose{ .Eye = eye, .Forward = VecMath::Normalize(VecMath::Subtract(shot.Center, eye), Vec3{ 1.0f, 0.0f, 0.0f }) };
    }

    auto YawPitchOf(const Vec3& forward) -> ShotYawPitch
    {
        const float length{ VecMath::Length(forward) };
        if (!(length > 1e-6f) || !std::isfinite(length)) return ShotYawPitch{};

        return ShotYawPitch{ .Yaw = std::atan2(forward.Y, forward.X), .Pitch = std::asin(std::clamp(forward.Z / length, -1.0f, 1.0f)) };
    }

    auto OrbitFromPose(const Vec3& eye, const Vec3& forward, const std::optional<Vec3>& hit) -> Shot
    {
        Vec3 center{};

        if (hit && VecMath::IsFinite(*hit))
        {
            center = *hit;
        }
        else
        {
            const Vec3 direction{ VecMath::Normalize(forward, Vec3{ 1.0f, 0.0f, 0.0f }) };

            float distance{ k_NoHitDistance };
            if (direction.Z < -1e-4f) distance = (std::min)(k_NoHitDrop / -direction.Z, k_MaxCenterDistance);

            center = VecMath::Add(eye, VecMath::Scale(direction, distance));
        }

        const Vec3 offset{ VecMath::Subtract(eye, center) };
        const float radius{ std::hypot(offset.X, offset.Y) };

        Shot shot{};
        shot.Kind = ShotKind::Orbit;
        shot.Center = center;
        shot.Radius = radius;
        shot.Height = offset.Z;
        shot.StartDegrees = radius > 1e-6f ? std::atan2(offset.Y, offset.X) * k_DegreesPerRadian : 0.0f;
        return shot;
    }

    auto IsRoundChange(const std::optional<RoundKey>& last, const RoundKey& now) -> bool
    {
        return last.has_value() && *last != now;
    }

    auto AddRoundChange(std::uint32_t counted, bool isRoundChanged, double elapsedSeconds) -> std::uint32_t
    {
        return isRoundChanged && elapsedSeconds >= k_NoDoubleCutSeconds ? counted + 1 : counted;
    }

    auto Advance(const AdvanceInput& input) -> std::optional<std::size_t>
    {
        if (input.Count == 0) return std::nullopt;
        if (input.Index >= input.Count) return std::size_t{ 0 };

        const std::size_t next{ (input.Index + 1) % input.Count };

        switch (input.End.Kind)
        {
        case ShotEndKind::Seconds:
        {
            const double seconds{ IsShotEndValid(input.End) ? static_cast<double>(input.End.Value) : static_cast<double>(k_DefaultShotSeconds) };
            return input.ElapsedSeconds >= seconds ? std::optional<std::size_t>{ next } : std::nullopt;
        }

        case ShotEndKind::Rounds:
        {
            const double rounds{ IsShotEndValid(input.End) ? static_cast<double>(input.End.Value) : static_cast<double>(k_DefaultShotRounds) };
            return static_cast<double>(input.RoundChanges) >= rounds ? std::optional<std::size_t>{ next } : std::nullopt;
        }

        case ShotEndKind::Hold:
            return std::nullopt;
        }

        return std::nullopt;
    }

    auto DescribeShotProgress(const ShotEnd& end, double elapsedSeconds, std::uint32_t roundChanges) -> std::string
    {
        switch (end.Kind)
        {
        case ShotEndKind::Seconds: return std::format("{:.1f}/{:g} s", elapsedSeconds, end.Value);
        case ShotEndKind::Rounds: return std::format("{:.1f} s, round {}/{:g}", elapsedSeconds, roundChanges, end.Value);
        case ShotEndKind::Hold: return std::format("{:.1f} s, hold", elapsedSeconds);
        }

        return {};
    }

    auto PickPovAgent(std::span<const RosterEntry> roster, std::uint32_t lastHandle) -> std::optional<std::uint32_t>
    {
        if (roster.empty()) return std::nullopt;

        std::size_t start{ 0 };
        for (std::size_t i{ 0 }; i < roster.size(); ++i)
        {
            if (roster[i].Handle == lastHandle)
            {
                start = i + 1;
                break;
            }
        }

        for (std::size_t offset{ 0 }; offset < roster.size(); ++offset)
        {
            const RosterEntry& entry{ roster[(start + offset) % roster.size()] };
            if (entry.BotNumber.has_value() && entry.IsAlive) return entry.Handle;
        }

        return std::nullopt;
    }

    auto NextPlayableShot(const ShotList& shots, const ShotIndexList& playlist, std::size_t first, int direction, bool canPov) -> std::optional<std::size_t>
    {
        if (playlist.empty()) return std::nullopt;

        const std::ptrdiff_t count{ static_cast<std::ptrdiff_t>(playlist.size()) };
        const std::ptrdiff_t step{ direction < 0 ? -1 : 1 };
        std::ptrdiff_t position{ first >= playlist.size() ? 0 : static_cast<std::ptrdiff_t>(first) };

        for (std::ptrdiff_t visited{ 0 }; visited < count; ++visited)
        {
            const std::size_t libraryIndex{ playlist[static_cast<std::size_t>(position)] };

            if (libraryIndex < shots.size())
            {
                const Shot& shot{ shots[libraryIndex] };
                if (IsShotValid(shot) && (shot.Kind != ShotKind::Pov || canPov)) return static_cast<std::size_t>(position);
            }

            position = (position + step + count) % count;
        }

        return std::nullopt;
    }

    auto DecidePovRequest(bool isDirectorPlaying, std::uint32_t directedPlayer, bool isWatchPovOn, std::uint32_t observed) -> std::optional<std::uint32_t>
    {
        if (isDirectorPlaying) return directedPlayer != k_NoHandle ? std::optional<std::uint32_t>{ directedPlayer } : std::nullopt;
        if (isWatchPovOn && observed != k_NoHandle) return observed;

        return std::nullopt;
    }

    auto SanitizeMapKey(std::string_view name) -> std::string
    {
        std::string key{};

        for (const char c : name)
        {
            const bool isKept{ (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' };
            key += isKept ? c : '_';
            if (key.size() >= k_MaxKeyLength) break;
        }

        if (key.empty()) return "default";

        std::size_t dots{ 0 };
        while (dots < key.size() && key[dots] == '.') ++dots;
        std::fill_n(key.begin(), dots, '_');

        return key;
    }
}