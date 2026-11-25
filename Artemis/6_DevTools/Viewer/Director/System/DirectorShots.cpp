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
    using DevTools::Viewer::Director::Type::ParsedShots;
    using DevTools::Viewer::Director::Type::RoundKey;
    using DevTools::Viewer::Director::Type::Shot;
    using DevTools::Viewer::Director::Type::ShotKind;
    using DevTools::Viewer::Director::Type::ShotList;
    using DevTools::Viewer::Director::Type::ShotPose;
    using DevTools::Viewer::Director::Type::ShotYawPitch;
    using DevTools::Viewer::Roster::Type::RosterEntry;

    using Common::Engine::Type::k_NoHandle;
    using DevTools::Viewer::Director::Type::k_DefaultShotSeconds;
    using DevTools::Viewer::Director::Type::k_MaxCenterDistance;
    using DevTools::Viewer::Director::Type::k_NoDoubleCutSeconds;
    using DevTools::Viewer::Director::Type::k_NoHitDistance;
    using DevTools::Viewer::Director::Type::k_NoHitDrop;

    constexpr float k_Pi{ std::numbers::pi_v<float> };
    constexpr float k_RadiansPerDegree{ k_Pi / 180.0f };
    constexpr float k_DegreesPerRadian{ 180.0f / k_Pi };
    constexpr std::size_t k_MaxKeyLength{ 64 };

    constexpr std::string_view k_FileHeader{
        "# Artemis director shots of this map. One shot per line, fields separated by '|', numbers with '.', yaw and pitch in degrees.\n"
        "#   fixed|<name>|<seconds>|<x> <y> <z>|<yaw> <pitch>\n"
        "#   orbit|<name>|<seconds>|<cx> <cy> <cz>|<radius> <height> <start degrees> <degrees per second>\n"
        "#   pov|<name>|<seconds>\n" };

    // return: Whether the character separates the numbers of a field.
    auto IsBlank(char c) -> bool
    {
        return c == ' ' || c == '\t' || c == '\r' || c == '\n';
    }

    auto Trim(std::string_view text) -> std::string_view
    {
        while (!text.empty() && IsBlank(text.front())) text.remove_prefix(1);
        while (!text.empty() && IsBlank(text.back())) text.remove_suffix(1);
        return text;
    }

    auto SplitOn(std::string_view text, char separator) -> std::vector<std::string_view>
    {
        std::vector<std::string_view> parts{};

        std::size_t start{ 0 };
        while (true)
        {
            const std::size_t end{ text.find(separator, start) };
            if (end == std::string_view::npos)
            {
                parts.push_back(text.substr(start));
                return parts;
            }

            parts.push_back(text.substr(start, end - start));
            start = end + 1;
        }
    }

    // return: Whether the text holds exactly out.size() finite numbers separated by blanks. They are written to out.
    auto ParseNumbers(std::string_view text, std::span<float> out) -> bool
    {
        std::size_t count{ 0 };
        std::size_t position{ 0 };

        while (true)
        {
            while (position < text.size() && IsBlank(text[position])) ++position;
            if (position >= text.size()) break;

            std::size_t end{ position };
            while (end < text.size() && !IsBlank(text[end])) ++end;
            if (count >= out.size()) return false;

            float value{ 0.0f };
            const char* first{ text.data() + position };
            const char* last{ text.data() + end };
            const std::from_chars_result result{ std::from_chars(first, last, value) };
            if (result.ec != std::errc{} || result.ptr != last || !std::isfinite(value)) return false;

            out[count++] = value;
            position = end;
        }

        return count == out.size();
    }

    // return: The shot of one line, or nothing when the line does not parse or the shot cannot be played.
    auto ParseLine(std::string_view line) -> std::optional<Shot>
    {
        const std::vector<std::string_view> fields{ SplitOn(line, '|') };
        if (fields.size() < 3) return std::nullopt;

        std::string kind{ Trim(fields[0]) };
        std::ranges::transform(kind, kind.begin(), [](char c) { return static_cast<char>(std::tolower(static_cast<unsigned char>(c))); });

        Shot shot{};
        shot.Name = std::string{ Trim(fields[1]) };

        std::array<float, 1> seconds{};
        if (!ParseNumbers(fields[2], seconds)) return std::nullopt;
        shot.Seconds = seconds[0];

        if (kind == "pov")
        {
            if (fields.size() != 3) return std::nullopt;
            shot.Kind = ShotKind::Pov;
        }
        else if (kind == "fixed")
        {
            std::array<float, 3> eye{};
            std::array<float, 2> angles{};
            if (fields.size() != 5 || !ParseNumbers(fields[3], eye) || !ParseNumbers(fields[4], angles)) return std::nullopt;

            shot.Kind = ShotKind::Fixed;
            shot.Eye = Vec3{ eye[0], eye[1], eye[2] };
            shot.Yaw = angles[0] * k_RadiansPerDegree;
            shot.Pitch = angles[1] * k_RadiansPerDegree;
        }
        else if (kind == "orbit")
        {
            std::array<float, 3> center{};
            std::array<float, 4> turn{};
            if (fields.size() != 5 || !ParseNumbers(fields[3], center) || !ParseNumbers(fields[4], turn)) return std::nullopt;

            shot.Kind = ShotKind::Orbit;
            shot.Center = Vec3{ center[0], center[1], center[2] };
            shot.Radius = turn[0];
            shot.Height = turn[1];
            shot.StartDegrees = turn[2];
            shot.DegreesPerSecond = turn[3];
        }
        else
        {
            return std::nullopt;
        }

        if (!DevTools::Viewer::Director::System::IsShotValid(shot)) return std::nullopt;
        return shot;
    }

    // return: The name without the characters that would break the line.
    auto CleanName(std::string_view name) -> std::string
    {
        std::string clean{ name };
        std::ranges::replace_if(clean, [](char c) { return c == '|' || c == '\r' || c == '\n'; }, ' ');
        return std::string{ Trim(clean) };
    }
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

    auto IsShotValid(const Shot& shot) -> bool
    {
        if (!(shot.Seconds > 0.0f) || !std::isfinite(shot.Seconds)) return false;

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

    auto Advance(const AdvanceInput& input) -> std::optional<std::size_t>
    {
        if (input.Count == 0) return std::nullopt;
        if (input.Index >= input.Count) return std::size_t{ 0 };

        const std::size_t next{ (input.Index + 1) % input.Count };
        const double duration{ input.Seconds > 0.0f && std::isfinite(input.Seconds) ? static_cast<double>(input.Seconds) : static_cast<double>(k_DefaultShotSeconds) };

        if (input.ElapsedSeconds >= duration) return next;
        if (input.IsRoundChanged && input.IsCutOnRound && input.ElapsedSeconds >= k_NoDoubleCutSeconds) return next;

        return std::nullopt;
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

    auto NextPlayableShot(const ShotList& shots, std::size_t first, int direction, bool canPov) -> std::optional<std::size_t>
    {
        if (shots.empty()) return std::nullopt;

        const std::ptrdiff_t count{ static_cast<std::ptrdiff_t>(shots.size()) };
        const std::ptrdiff_t step{ direction < 0 ? -1 : 1 };
        std::ptrdiff_t index{ first >= shots.size() ? 0 : static_cast<std::ptrdiff_t>(first) };

        for (std::ptrdiff_t visited{ 0 }; visited < count; ++visited)
        {
            const Shot& shot{ shots[static_cast<std::size_t>(index)] };
            if (IsShotValid(shot) && (shot.Kind != ShotKind::Pov || canPov)) return static_cast<std::size_t>(index);

            index = (index + step + count) % count;
        }

        return std::nullopt;
    }

    auto DecidePovRequest(bool isDirectorPlaying, std::uint32_t directedPlayer, bool isWatchPovOn, std::uint32_t observed) -> std::optional<std::uint32_t>
    {
        if (isDirectorPlaying) return directedPlayer != k_NoHandle ? std::optional<std::uint32_t>{ directedPlayer } : std::nullopt;
        if (isWatchPovOn && observed != k_NoHandle) return observed;

        return std::nullopt;
    }

    auto ParseShots(std::string_view text) -> ParsedShots
    {
        ParsedShots parsed{};

        for (const std::string_view rawLine : SplitOn(text, '\n'))
        {
            const std::string_view line{ Trim(rawLine) };
            if (line.empty() || line.front() == '#') continue;

            std::optional<Shot> shot{ ParseLine(line) };
            if (shot) parsed.Shots.push_back(std::move(*shot));
            else ++parsed.Skipped;
        }

        return parsed;
    }

    auto FormatShots(const ShotList& shots) -> std::string
    {
        std::string text{ k_FileHeader };

        for (const Shot& shot : shots)
        {
            const std::string name{ CleanName(shot.Name) };

            switch (shot.Kind)
            {
            case ShotKind::Fixed:
                text += std::format("fixed|{}|{}|{:.3f} {:.3f} {:.3f}|{:.3f} {:.3f}\n", name, shot.Seconds, shot.Eye.X, shot.Eye.Y, shot.Eye.Z,
                    shot.Yaw * k_DegreesPerRadian, shot.Pitch * k_DegreesPerRadian);
                break;

            case ShotKind::Orbit:
                text += std::format("orbit|{}|{}|{:.3f} {:.3f} {:.3f}|{:.3f} {:.3f} {:.3f} {}\n", name, shot.Seconds, shot.Center.X, shot.Center.Y,
                    shot.Center.Z, shot.Radius, shot.Height, shot.StartDegrees, shot.DegreesPerSecond);
                break;

            case ShotKind::Pov:
                text += std::format("pov|{}|{}\n", name, shot.Seconds);
                break;
            }
        }

        return text;
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