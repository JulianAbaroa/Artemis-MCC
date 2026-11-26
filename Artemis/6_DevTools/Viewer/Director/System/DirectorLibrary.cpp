module DevTools.Viewer.Director.System;
import :Library;
import :Shots;

import Common.Math.Type;
import DevTools.Viewer.Director.Type;
import std;

namespace
{
    using DevTools::Viewer::Director::System::IsShotEndValid;
    using DevTools::Viewer::Director::System::IsShotValid;
    using DevTools::Viewer::Director::System::SanitizeName;
    using DevTools::Viewer::Director::Type::DirectorFile;
    using DevTools::Viewer::Director::Type::ParsedDirector;
    using DevTools::Viewer::Director::Type::Sequence;
    using DevTools::Viewer::Director::Type::Shot;
    using DevTools::Viewer::Director::Type::ShotEnd;
    using DevTools::Viewer::Director::Type::ShotEndKind;
    using DevTools::Viewer::Director::Type::ShotIdList;
    using DevTools::Viewer::Director::Type::ShotIndexList;
    using DevTools::Viewer::Director::Type::ShotKind;
    using DevTools::Viewer::Director::Type::ShotList;

    using Common::Math::Type::k_DegreesPerRadian;
    using Common::Math::Type::k_RadiansPerDegree;

    constexpr std::string_view k_VersionLine{ "version 2" };
    constexpr std::string_view k_DefaultSequenceName{ "Sequence" };

    constexpr std::string_view k_FileHeader{
        "version 2\n"
        "# Artemis director file of this map. One item per line, fields separated by '|', numbers with '.', yaw and pitch in degrees.\n"
        "# The end of a shot is 's <seconds>', 'r <rounds>' or 'hold'.\n"
        "#   shot|<id>|fixed|<name>|<on|off>|<end>|<x> <y> <z>|<yaw> <pitch>\n"
        "#   shot|<id>|orbit|<name>|<on|off>|<end>|<cx> <cy> <cz>|<radius> <height> <start degrees> <degrees per second>\n"
        "#   shot|<id>|pov|<name>|<on|off>|<end>\n"
        "#   sequence|<name>|<id> <id> ...\n"
        "#   active|<name>\n" };

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

    auto Lower(std::string_view text) -> std::string
    {
        std::string lower{ text };
        std::ranges::transform(lower, lower.begin(), [](char c) { return static_cast<char>(std::tolower(static_cast<unsigned char>(c))); });
        return lower;
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

    // return: The blank separated words of the text.
    auto Words(std::string_view text) -> std::vector<std::string_view>
    {
        std::vector<std::string_view> words{};

        std::size_t position{ 0 };
        while (true)
        {
            while (position < text.size() && IsBlank(text[position])) ++position;
            if (position >= text.size()) return words;

            std::size_t end{ position };
            while (end < text.size() && !IsBlank(text[end])) ++end;

            words.push_back(text.substr(position, end - position));
            position = end;
        }
    }

    // return: Whether the text holds exactly out.size() finite numbers separated by blanks. They are written to out.
    auto ParseNumbers(std::string_view text, std::span<float> out) -> bool
    {
        const std::vector<std::string_view> words{ Words(text) };
        if (words.size() != out.size()) return false;

        for (std::size_t i{ 0 }; i < words.size(); ++i)
        {
            float value{ 0.0f };
            const char* first{ words[i].data() };
            const char* last{ words[i].data() + words[i].size() };
            const std::from_chars_result result{ std::from_chars(first, last, value) };
            if (result.ec != std::errc{} || result.ptr != last || !std::isfinite(value)) return false;

            out[i] = value;
        }

        return true;
    }

    // return: The whole number of the text, or nothing when it is not one.
    auto ParseUnsigned(std::string_view text) -> std::optional<std::uint32_t>
    {
        const std::string_view word{ Trim(text) };
        std::uint32_t value{ 0 };
        const char* last{ word.data() + word.size() };
        const std::from_chars_result result{ std::from_chars(word.data(), last, value) };

        if (word.empty() || result.ec != std::errc{} || result.ptr != last) return std::nullopt;
        return value;
    }

    // return: The end of the text ("s 20", "r 2" or "hold"), or nothing when it is not a valid one.
    auto ParseEnd(std::string_view text) -> std::optional<ShotEnd>
    {
        const std::vector<std::string_view> words{ Words(text) };
        if (words.empty()) return std::nullopt;

        const std::string kind{ Lower(words[0]) };

        if (kind == "hold") return words.size() == 1 ? std::optional<ShotEnd>{ ShotEnd{ .Kind = ShotEndKind::Hold, .Value = 0.0f } } : std::nullopt;
        if (words.size() != 2 || (kind != "s" && kind != "r")) return std::nullopt;

        std::array<float, 1> value{};
        if (!ParseNumbers(words[1], value)) return std::nullopt;

        const ShotEnd end{ .Kind = kind == "s" ? ShotEndKind::Seconds : ShotEndKind::Rounds, .Value = value[0] };
        return IsShotEndValid(end) ? std::optional<ShotEnd>{ end } : std::nullopt;
    }

    // return: The text of an end as ParseEnd reads it.
    auto FormatEnd(const ShotEnd& end) -> std::string
    {
        switch (end.Kind)
        {
        case ShotEndKind::Seconds: return std::format("s {}", end.Value);
        case ShotEndKind::Rounds: return std::format("r {}", static_cast<std::int64_t>(end.Value));
        case ShotEndKind::Hold: return "hold";
        }

        return "hold";
    }

    // Reads the pose fields of a shot of the kind into it. return: False when the fields do not fit the kind.
    auto ParsePose(std::string_view kind, std::span<const std::string_view> poseFields, Shot& shot) -> bool
    {
        if (kind == "pov")
        {
            shot.Kind = ShotKind::Pov;
            return poseFields.empty();
        }

        if (poseFields.size() != 2) return false;

        if (kind == "fixed")
        {
            std::array<float, 3> eye{};
            std::array<float, 2> angles{};
            if (!ParseNumbers(poseFields[0], eye) || !ParseNumbers(poseFields[1], angles)) return false;

            shot.Kind = ShotKind::Fixed;
            shot.Eye = { eye[0], eye[1], eye[2] };
            shot.Yaw = angles[0] * k_RadiansPerDegree;
            shot.Pitch = angles[1] * k_RadiansPerDegree;
            return true;
        }

        if (kind == "orbit")
        {
            std::array<float, 3> center{};
            std::array<float, 4> turn{};
            if (!ParseNumbers(poseFields[0], center) || !ParseNumbers(poseFields[1], turn)) return false;

            shot.Kind = ShotKind::Orbit;
            shot.Center = { center[0], center[1], center[2] };
            shot.Radius = turn[0];
            shot.Height = turn[1];
            shot.StartDegrees = turn[2];
            shot.DegreesPerSecond = turn[3];
            return true;
        }

        return false;
    }

    // return: The shot of a line of the current version, or nothing when it does not parse or cannot be played.
    auto ParseShotLine(std::span<const std::string_view> fields) -> std::optional<Shot>
    {
        // shot|<id>|<kind>|<name>|<on|off>|<end>|<pose fields>
        constexpr std::size_t k_CommonFields{ 6 };
        if (fields.size() < k_CommonFields) return std::nullopt;

        const std::optional<std::uint32_t> id{ ParseUnsigned(fields[1]) };
        const std::string onOff{ Lower(Trim(fields[4])) };
        const std::optional<ShotEnd> end{ ParseEnd(fields[5]) };
        if (!id || *id == 0 || (onOff != "on" && onOff != "off") || !end) return std::nullopt;

        Shot shot{ .Id = *id, .Name = SanitizeName(fields[3]), .IsEnabled = onOff == "on", .End = *end };
        if (!ParsePose(Lower(Trim(fields[2])), fields.subspan(k_CommonFields), shot)) return std::nullopt;

        return IsShotValid(shot) ? std::optional<Shot>{ shot } : std::nullopt;
    }

    // return: The shot of a line of the first version (kind|name|seconds|...), without an id.
    auto ParseFirstVersionLine(std::span<const std::string_view> fields) -> std::optional<Shot>
    {
        constexpr std::size_t k_CommonFields{ 3 };
        if (fields.size() < k_CommonFields) return std::nullopt;

        std::array<float, 1> seconds{};
        if (!ParseNumbers(fields[2], seconds)) return std::nullopt;

        Shot shot{ .Name = SanitizeName(fields[1]), .End = ShotEnd{ .Kind = ShotEndKind::Seconds, .Value = seconds[0] } };
        if (!ParsePose(Lower(Trim(fields[0])), fields.subspan(k_CommonFields), shot)) return std::nullopt;

        return IsShotValid(shot) ? std::optional<Shot>{ shot } : std::nullopt;
    }

    // return: The ids of the text, or nothing when a word is not an id.
    auto ParseIds(std::string_view text) -> std::optional<ShotIdList>
    {
        ShotIdList ids{};

        for (const std::string_view word : Words(text))
        {
            const std::optional<std::uint32_t> id{ ParseUnsigned(word) };
            if (!id) return std::nullopt;

            ids.push_back(*id);
        }

        return ids;
    }

    // Moves the element at from to the place to.
    template <typename Container>
    auto MoveWithin(Container& items, std::size_t from, std::size_t to) -> bool
    {
        if (from >= items.size() || to >= items.size() || from == to) return false;

        const auto begin{ items.begin() };
        if (from < to) std::rotate(begin + static_cast<std::ptrdiff_t>(from), begin + static_cast<std::ptrdiff_t>(from) + 1, begin + static_cast<std::ptrdiff_t>(to) + 1);
        else std::rotate(begin + static_cast<std::ptrdiff_t>(to), begin + static_cast<std::ptrdiff_t>(from), begin + static_cast<std::ptrdiff_t>(from) + 1);

        return true;
    }

    // return: The name the sequence gets: the wanted one, or the wanted one and a number when another sequence has it.
    auto UniqueSequenceName(const DirectorFile& file, std::string_view wanted, const Sequence* except) -> std::string
    {
        const std::string base{ wanted.empty() ? std::string{ k_DefaultSequenceName } : std::string{ wanted } };

        const auto isTaken{ [&](const std::string& name)
        {
            return std::ranges::any_of(file.Sequences, [&](const Sequence& other) { return &other != except && other.Name == name; });
        } };

        std::string name{ base };
        for (int number{ 2 }; isTaken(name); ++number) name = std::format("{} {}", base, number);

        return name;
    }

    // Reads the lines of the current version into the file. The sequences are resolved by the caller once every shot is known.
    struct RawSequence
    {
        std::string Name{};
        ShotIdList Ids{};
    };
}

namespace DevTools::Viewer::Director::System
{
    auto SanitizeName(std::string_view name) -> std::string
    {
        std::string clean{ name };
        std::ranges::replace_if(clean, [](char c) { return c == '|' || c == '\r' || c == '\n'; }, ' ');
        return std::string{ Trim(clean) };
    }

    auto ParseSequenceRequest(std::string_view value) -> std::optional<std::string>
    {
        std::string name{ SanitizeName(value) };
        if (name.empty()) return std::nullopt;

        return name;
    }

    auto ParseDirectorFile(std::string_view text) -> ParsedDirector
    {
        ParsedDirector parsed{};
        const std::vector<std::string_view> lines{ SplitOn(text, '\n') };

        bool isFirstSignificant{ true };
        bool isCurrentVersion{ false };
        bool isAnyLine{ false };
        std::vector<RawSequence> rawSequences{};
        std::string active{};

        for (const std::string_view rawLine : lines)
        {
            const std::string_view line{ Trim(rawLine) };
            if (line.empty() || line.front() == '#') continue;

            isAnyLine = true;

            if (isFirstSignificant)
            {
                isFirstSignificant = false;
                if (line == k_VersionLine)
                {
                    isCurrentVersion = true;
                    continue;
                }

                // note: A version this code does not know is read as the current one, and its line counts as skipped.
                if (Lower(line).starts_with("version"))
                {
                    isCurrentVersion = true;
                    ++parsed.Skipped;
                    continue;
                }
            }

            const std::vector<std::string_view> fields{ SplitOn(line, '|') };

            if (!isCurrentVersion)
            {
                std::optional<Shot> shot{ ParseFirstVersionLine(fields) };
                if (!shot)
                {
                    ++parsed.Skipped;
                    continue;
                }

                shot->Id = static_cast<std::uint32_t>(parsed.File.Shots.size() + 1);
                parsed.File.Shots.push_back(std::move(*shot));
                continue;
            }

            const std::string kind{ Lower(Trim(fields[0])) };

            if (kind == "shot")
            {
                std::optional<Shot> shot{ ParseShotLine(fields) };
                const bool isDuplicate{ shot && std::ranges::any_of(parsed.File.Shots, [&](const Shot& other) { return other.Id == shot->Id; }) };

                if (shot && !isDuplicate) parsed.File.Shots.push_back(std::move(*shot));
                else ++parsed.Skipped;
            }
            else if (kind == "sequence" && fields.size() == 3)
            {
                std::string name{ SanitizeName(fields[1]) };
                std::optional<ShotIdList> ids{ ParseIds(fields[2]) };
                const bool isDuplicate{ std::ranges::any_of(rawSequences, [&](const RawSequence& other) { return other.Name == name; }) };

                if (!name.empty() && ids && !isDuplicate) rawSequences.push_back(RawSequence{ .Name = std::move(name), .Ids = std::move(*ids) });
                else ++parsed.Skipped;
            }
            else if (kind == "active" && fields.size() == 2)
            {
                active = SanitizeName(fields[1]);
            }
            else
            {
                ++parsed.Skipped;
            }
        }

        parsed.IsMigrated = isAnyLine && !isCurrentVersion;

        for (RawSequence& raw : rawSequences)
        {
            Sequence sequence{ .Name = std::move(raw.Name) };

            for (const std::uint32_t id : raw.Ids)
            {
                if (FindShotIndex(parsed.File, id)) sequence.ShotIds.push_back(id);
                else ++parsed.DroppedIds;
            }

            parsed.File.Sequences.push_back(std::move(sequence));
        }

        if (FindSequence(parsed.File, active)) parsed.File.ActiveSequence = active;

        return parsed;
    }

    auto FormatDirectorFile(const DirectorFile& file) -> std::string
    {
        std::string text{ k_FileHeader };

        for (const Shot& shot : file.Shots)
        {
            const std::string_view onOff{ shot.IsEnabled ? "on" : "off" };
            const std::string name{ SanitizeName(shot.Name) };
            const std::string end{ FormatEnd(shot.End) };

            switch (shot.Kind)
            {
            case ShotKind::Fixed:
                text += std::format("shot|{}|fixed|{}|{}|{}|{:.3f} {:.3f} {:.3f}|{:.3f} {:.3f}\n", shot.Id, name, onOff, end, shot.Eye.X, shot.Eye.Y, shot.Eye.Z,
                    shot.Yaw * k_DegreesPerRadian, shot.Pitch * k_DegreesPerRadian);
                break;

            case ShotKind::Orbit:
                text += std::format("shot|{}|orbit|{}|{}|{}|{:.3f} {:.3f} {:.3f}|{:.3f} {:.3f} {:.3f} {}\n", shot.Id, name, onOff, end, shot.Center.X,
                    shot.Center.Y, shot.Center.Z, shot.Radius, shot.Height, shot.StartDegrees, shot.DegreesPerSecond);
                break;

            case ShotKind::Pov:
                text += std::format("shot|{}|pov|{}|{}|{}\n", shot.Id, name, onOff, end);
                break;
            }
        }

        for (const Sequence& sequence : file.Sequences)
        {
            std::string ids{};
            for (const std::uint32_t id : sequence.ShotIds) ids += std::format("{}{}", ids.empty() ? "" : " ", id);

            text += std::format("sequence|{}|{}\n", SanitizeName(sequence.Name), ids);
        }

        if (!file.ActiveSequence.empty()) text += std::format("active|{}\n", SanitizeName(file.ActiveSequence));

        return text;
    }

    auto NextShotIdOf(const DirectorFile& file) -> std::uint32_t
    {
        std::uint32_t highest{ 0 };
        for (const Shot& shot : file.Shots) highest = (std::max)(highest, shot.Id);

        return highest + 1;
    }

    auto FindShotIndex(const DirectorFile& file, std::uint32_t id) -> std::optional<std::size_t>
    {
        const auto found{ std::ranges::find(file.Shots, id, &Shot::Id) };
        if (found == file.Shots.end()) return std::nullopt;

        return static_cast<std::size_t>(found - file.Shots.begin());
    }

    auto FindSequence(const DirectorFile& file, std::string_view name) -> const Sequence*
    {
        if (name.empty()) return nullptr;

        const auto found{ std::ranges::find_if(file.Sequences, [name](const Sequence& sequence) { return sequence.Name == name; }) };
        return found != file.Sequences.end() ? &*found : nullptr;
    }

    auto FindSequence(DirectorFile& file, std::string_view name) -> Sequence*
    {
        return const_cast<Sequence*>(FindSequence(std::as_const(file), name));
    }

    auto Playlist(const DirectorFile& file) -> ShotIndexList
    {
        ShotIndexList playlist{};
        const Sequence* sequence{ FindSequence(file, file.ActiveSequence) };

        if (sequence)
        {
            for (const std::uint32_t id : sequence->ShotIds)
            {
                const std::optional<std::size_t> index{ FindShotIndex(file, id) };
                if (index && file.Shots[*index].IsEnabled) playlist.push_back(*index);
            }

            return playlist;
        }

        for (std::size_t i{ 0 }; i < file.Shots.size(); ++i)
        {
            if (file.Shots[i].IsEnabled) playlist.push_back(i);
        }

        return playlist;
    }

    auto MoveShot(ShotList& shots, std::size_t from, std::size_t to) -> bool
    {
        return MoveWithin(shots, from, to);
    }

    auto MoveInSequence(Sequence& sequence, std::size_t from, std::size_t to) -> bool
    {
        return MoveWithin(sequence.ShotIds, from, to);
    }

    auto AddToSequence(Sequence& sequence, std::uint32_t id, std::size_t at) -> bool
    {
        const std::size_t slot{ (std::min)(at, sequence.ShotIds.size()) };
        sequence.ShotIds.insert(sequence.ShotIds.begin() + static_cast<std::ptrdiff_t>(slot), id);
        return true;
    }

    auto RemoveFromSequence(Sequence& sequence, std::size_t index) -> bool
    {
        if (index >= sequence.ShotIds.size()) return false;

        sequence.ShotIds.erase(sequence.ShotIds.begin() + static_cast<std::ptrdiff_t>(index));
        return true;
    }

    auto AddShot(DirectorFile& file, Shot shot) -> void
    {
        const std::uint32_t id{ shot.Id };
        file.Shots.push_back(std::move(shot));

        if (Sequence* active{ FindSequence(file, file.ActiveSequence) }) active->ShotIds.push_back(id);
    }

    auto RemoveShot(DirectorFile& file, std::uint32_t id) -> bool
    {
        for (Sequence& sequence : file.Sequences) std::erase(sequence.ShotIds, id);

        const std::optional<std::size_t> index{ FindShotIndex(file, id) };
        if (!index) return false;

        file.Shots.erase(file.Shots.begin() + static_cast<std::ptrdiff_t>(*index));
        return true;
    }

    auto NewSequence(DirectorFile& file, std::string_view name) -> std::string
    {
        std::string unique{ UniqueSequenceName(file, SanitizeName(name), nullptr) };
        file.Sequences.push_back(Sequence{ .Name = unique });
        return unique;
    }

    auto RenameSequence(DirectorFile& file, std::string_view name, std::string_view newName) -> std::optional<std::string>
    {
        Sequence* sequence{ FindSequence(file, name) };
        const std::string clean{ SanitizeName(newName) };
        if (!sequence || clean.empty()) return std::nullopt;

        const bool isActive{ file.ActiveSequence == sequence->Name };
        sequence->Name = UniqueSequenceName(file, clean, sequence);

        if (isActive) file.ActiveSequence = sequence->Name;
        return sequence->Name;
    }

    auto DeleteSequence(DirectorFile& file, std::string_view name) -> bool
    {
        const Sequence* sequence{ FindSequence(file, name) };
        if (!sequence) return false;

        if (file.ActiveSequence == sequence->Name) file.ActiveSequence.clear();
        file.Sequences.erase(file.Sequences.begin() + (sequence - file.Sequences.data()));
        return true;
    }
}