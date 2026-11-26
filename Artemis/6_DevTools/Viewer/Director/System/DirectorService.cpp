module DevTools.Viewer.Director.System;
import :Shots;
import :Library;

import Common.Engine.Type;
import DevTools.Viewer.Roster.Type;
import DevTools.Viewer.Director.Type;
import std;

namespace
{
    using std::chrono::duration;
    using std::chrono::steady_clock;

    using DevTools::Viewer::Director::Type::AdvanceInput;
    using DevTools::Viewer::Director::Type::DirectorFileInfo;
    using DevTools::Viewer::Director::Type::DirectorStatus;
    using DevTools::Viewer::Director::Type::ParsedDirector;
    using DevTools::Viewer::Director::Type::RoundKey;
    using DevTools::Viewer::Director::Type::Shot;
    using DevTools::Viewer::Director::Type::ShotKind;
    using DevTools::Viewer::Director::Type::ShotPose;
    using DevTools::Viewer::Director::Type::ShotYawPitch;
    using DevTools::Viewer::Roster::Type::RosterEntry;

    using Common::Engine::Type::k_NoHandle;
    using DevTools::Viewer::Director::Type::k_RetrySeconds;

    constexpr std::string_view k_DirectorFolder{ "Director" };
    constexpr std::string_view k_FileExtension{ ".txt" };
    constexpr std::string_view k_BackupExtension{ ".v1.bak" };

    // return: The file of the map inside the Artemis folder.
    auto FileOf(const std::string& directory, const std::string& key) -> std::filesystem::path
    {
        return std::filesystem::path{ directory } / k_DirectorFolder / (key + std::string{ k_FileExtension });
    }

    // return: The words for the panel after a load.
    auto LoadedMessage(const ParsedDirector& parsed) -> std::string
    {
        std::string message{ std::format("loaded {} shot(s), {} sequence(s)", parsed.File.Shots.size(), parsed.File.Sequences.size()) };
        if (parsed.Skipped > 0) message += std::format(", {} invalid line(s) skipped", parsed.Skipped);
        if (parsed.DroppedIds > 0) message += std::format(", {} unknown shot id(s) dropped", parsed.DroppedIds);
        if (parsed.IsMigrated) message += ", migrated from the first version (saved as the new one at the next edit)";
        return message;
    }

    // return: The time point that is the given real seconds after the start.
    auto After(steady_clock::time_point start, double seconds) -> steady_clock::time_point
    {
        return start + std::chrono::duration_cast<steady_clock::duration>(duration<double>(seconds));
    }
}

namespace DevTools::Viewer::Director::System
{
    auto DirectorService::Persist(const std::string& directory) -> void
    {
        const std::string key{ SanitizeMapKey(m_Store.GetMapName()) };

        if (!m_IsLoaded || key != m_LoadedKey || directory != m_LoadedDirectory)
        {
            m_IsLoaded = true;
            m_LoadedKey = key;
            m_LoadedDirectory = directory;

            this->Load(directory, key);
            return;
        }

        if (m_Store.GetRevision() != m_SavedRevision) this->Save(directory, key);
    }

    auto DirectorService::Load(const std::string& directory, const std::string& key) -> void
    {
        DirectorFileInfo info{ .MapKey = key };
        m_IsFirstVersionOnDisk = false;

        if (directory.empty())
        {
            m_Store.SetFile(DirectorFile{});
            info.Message = "no Artemis folder (AppData is off): the shots are not saved";
        }
        else
        {
            const std::filesystem::path path{ FileOf(directory, key) };
            info.Path = path.string();

            std::ifstream file{ path, std::ios::binary };
            if (!file)
            {
                m_Store.SetFile(DirectorFile{});
                info.Message = "no file yet";
            }
            else
            {
                const std::string text{ std::istreambuf_iterator<char>{ file }, std::istreambuf_iterator<char>{} };
                ParsedDirector parsed{ ParseDirectorFile(text) };

                info.Message = LoadedMessage(parsed);
                m_IsFirstVersionOnDisk = parsed.IsMigrated;
                m_Store.SetFile(std::move(parsed.File));
            }
        }

        m_SavedRevision = m_Store.GetRevision();
        m_Store.SetFileInfo(std::move(info));
    }

    auto DirectorService::Save(const std::string& directory, const std::string& key) -> void
    {
        // The revision is taken first: an edit that lands while the file is written is saved by the next pass.
        m_SavedRevision = m_Store.GetRevision();

        DirectorFileInfo info{ .MapKey = key };
        if (directory.empty())
        {
            info.Message = "no Artemis folder (AppData is off): the shots are not saved";
            m_Store.SetFileInfo(std::move(info));
            return;
        }

        const DirectorFile file{ m_Store.CopyFile() };
        const std::filesystem::path path{ FileOf(directory, key) };
        info.Path = path.string();

        std::error_code error{};
        std::filesystem::create_directories(path.parent_path(), error);

        // note: The first save over a file of the first version keeps that text aside once, so the migration can be undone by hand.
        if (m_IsFirstVersionOnDisk && !error)
        {
            std::filesystem::path backup{ path };
            backup += k_BackupExtension;

            std::error_code backupError{};
            if (!std::filesystem::exists(backup, backupError) && !backupError) std::filesystem::copy_file(path, backup, backupError);

            // note: The save goes on (the migration is not worth losing the edit), but the user is told once, because it is the only copy of the old text.
            if (backupError) m_Log.Message("[DirectorService] WARNING: Failed to keep the first version of the file as {} ({}), it is saved as the second one anyway.", backup.string(), backupError.message());
        }

        // note: Written aside and renamed over the file, so a crash never leaves half a file.
        std::filesystem::path temporary{ path };
        temporary += ".tmp";

        bool isWritten{ false };
        if (!error)
        {
            std::ofstream stream{ temporary, std::ios::binary | std::ios::trunc };
            stream << FormatDirectorFile(file);
            stream.flush();
            isWritten = static_cast<bool>(stream);
        }

        if (isWritten) std::filesystem::rename(temporary, path, error);

        const bool isSaved{ isWritten && !error };
        const bool isMigration{ isSaved && m_IsFirstVersionOnDisk };
        if (isSaved) m_IsFirstVersionOnDisk = false;

        // note: The save that writes the migration says so, because it replaces the "migrated" message of the load, which is the one place the user reads it.
        info.Message = !isSaved ? "save failed" : std::format("saved {} shot(s), {} sequence(s){}", file.Shots.size(), file.Sequences.size(),
            isMigration ? ", migrated from the first version (the old text is in the .v1.bak file)" : "");
        m_Store.SetFileInfo(std::move(info));
    }

    auto DirectorService::Observe(std::uint64_t matchId, bool isAutoPlay) -> bool
    {
        if (m_ObservedMatch && *m_ObservedMatch == matchId) return false;

        m_ObservedMatch = matchId;
        if (matchId == 0) return false;

        const std::optional<std::string> request{ m_Store.TakeLoadRequest(matchId) };
        if (!isAutoPlay && !request) return false;

        m_Store.RequestPlayFromStart(request.value_or(std::string{}));
        return true;
    }

    auto DirectorService::Stop() -> void
    {
        m_Store.SetPlaying(false);
        m_Store.ClearRequests();
        m_Store.SetStatus(DirectorStatus{});
        m_IsRunning = false;
        m_IsHolding = false;
        this->ClearRequestedSequence();
    }

    auto DirectorService::ClearRequestedSequence() -> void
    {
        if (m_RequestedSequence.empty()) return;

        m_RequestedSequence.clear();
        m_IsCacheDirty = true;
    }

    auto DirectorService::Refresh() -> bool
    {
        const std::uint64_t serial{ m_Store.GetLoadSerial() };
        const std::uint64_t revision{ m_Store.GetRevision() };
        const bool isLoaded{ serial != m_CacheLoadSerial };

        if (!isLoaded && revision == m_CacheRevision && !m_IsCacheDirty) return false;

        m_CacheLoadSerial = serial;
        m_CacheRevision = revision;
        m_IsCacheDirty = false;
        m_File = m_Store.CopyFile();

        // note: The sequence the bench asked for replaces the active one while it plays, without touching the file of the user. One that does not exist
        // is said once and the active one plays.
        if (!m_RequestedSequence.empty())
        {
            if (FindSequence(m_File, m_RequestedSequence))
            {
                m_File.ActiveSequence = m_RequestedSequence;
            }
            else if (m_WarnedSequence != m_RequestedSequence)
            {
                m_WarnedSequence = m_RequestedSequence;
                m_Log.Message("[DirectorService] WARNING: The sequence \"{}\" does not exist in the file of this map, so the active one plays.", m_RequestedSequence);
            }
        }

        m_Playlist = Playlist(m_File);
        return isLoaded;
    }

    auto DirectorService::LocateCurrent() -> std::optional<std::size_t>
    {
        const auto isCurrent{ [this](std::size_t position) { return m_Playlist[position] < m_File.Shots.size() && m_File.Shots[m_Playlist[position]].Id == m_ShotId; } };

        if (m_Index < m_Playlist.size() && isCurrent(m_Index)) return m_Index;

        for (std::size_t position{ 0 }; position < m_Playlist.size(); ++position)
        {
            if (!isCurrent(position)) continue;

            m_Index = position;
            return position;
        }

        return std::nullopt;
    }

    auto DirectorService::Begin(std::size_t first, int direction, const DirectorInput& input) -> void
    {
        const std::optional<std::uint32_t> agent{ input.HasRoster ? PickPovAgent(input.Roster, m_LastPovAgent) : std::nullopt };

        // note: Without a Tick the roster is not known yet. A POV shot that comes first waits for it instead of being skipped, so the playlist starts in
        // its order.
        const bool isWaitingForRoster{ !input.HasRoster && first < m_Playlist.size() && m_Playlist[first] < m_File.Shots.size() &&
            m_File.Shots[m_Playlist[first]].Kind == ShotKind::Pov && IsShotValid(m_File.Shots[m_Playlist[first]]) };

        const std::optional<std::size_t> position{ isWaitingForRoster ? std::nullopt : NextPlayableShot(m_File.Shots, m_Playlist, first, direction, agent.has_value()) };

        m_PovHandle = k_NoHandle;

        if (!position)
        {
            m_IsHolding = true;
            m_RetryAt = After(input.Now, input.HasRoster ? k_RetrySeconds : 0.0);
            return;
        }

        const Shot& shot{ m_File.Shots[m_Playlist[*position]] };

        m_IsHolding = false;
        m_Index = *position;
        m_ShotId = shot.Id;
        m_RoundChanges = 0;
        m_ShotStart = input.Now;

        if (shot.Kind == ShotKind::Pov)
        {
            m_PovHandle = *agent;
            m_LastPovAgent = *agent;
        }
    }

    auto DirectorService::Start(const DirectorInput& input, const std::optional<RoundKey>& round, bool isForNewMatch) -> bool
    {
        if (m_Playlist.empty() && !isForNewMatch) return false;

        m_IsRunning = true;

        // The round the playback starts in is the baseline, so it is not a change.
        m_LastRound = round;

        this->Begin(0, 1, input);
        return true;
    }

    auto DirectorService::Update(const DirectorInput& input) -> DirectorFrame
    {
        if (m_Store.ConsumeTogglePlay()) m_Store.SetPlaying(!m_Store.IsPlaying());

        const int steps{ m_Store.ConsumeStep() };
        const std::optional<std::string> fromStart{ m_Store.ConsumePlayFromStart() };

        // note: A new match forgets what the last one asked for: the warning of an unknown sequence is said once per match, and the sequence of the bench
        // does not stay for a match that did not ask for it (the request of this match, if any, comes with fromStart and sets it again).
        if (const std::uint64_t matchId{ m_Store.GetMatchId() }; matchId != m_UpdateMatchId)
        {
            m_UpdateMatchId = matchId;
            m_WarnedSequence.clear();
            if (!fromStart) this->ClearRequestedSequence();
        }

        if (!m_Store.IsPlaying())
        {
            m_IsRunning = false;
            m_IsHolding = false;
            this->ClearRequestedSequence();
            m_Store.SetStatus(DirectorStatus{});
            return DirectorFrame{};
        }

        if (fromStart)
        {
            m_RequestedSequence = *fromStart;
            m_IsCacheDirty = true;
            m_IsRunning = false;
        }

        const bool isLoaded{ this->Refresh() };

        const std::optional<RoundKey> round{ input.Round ? std::optional<RoundKey>{ RoundKey{ .MatchId = m_Store.GetMatchId(), .Round = *input.Round } } : std::nullopt };

        if (!m_IsRunning)
        {
            if (!this->Start(input, round, fromStart.has_value()))
            {
                m_Store.SetPlaying(false);
                m_Store.SetStatus(DirectorStatus{});
                return DirectorFrame{};
            }

            return this->Publish(input);
        }

        const bool isRoundChanged{ round.has_value() && IsRoundChange(m_LastRound, *round) };
        if (round) m_LastRound = round;

        const std::size_t count{ m_Playlist.size() };

        if (isLoaded)
        {
            // Another map: its own file, from the first shot.
            this->Begin(0, 1, input);
        }
        else if (steps != 0)
        {
            const std::ptrdiff_t size{ static_cast<std::ptrdiff_t>(count) };
            const std::ptrdiff_t target{ size == 0 ? 0 : ((static_cast<std::ptrdiff_t>(m_Index) + steps) % size + size) % size };
            this->Begin(static_cast<std::size_t>(target), steps > 0 ? 1 : -1, input);
        }
        else if (m_IsHolding)
        {
            if (input.Now >= m_RetryAt) this->Begin(0, 1, input);
        }
        else if (const std::optional<std::size_t> position{ this->LocateCurrent() }; !position)
        {
            // An edit took the shot that was playing out of the playlist: the one that took its place goes on.
            this->Begin(m_Index < count ? m_Index : 0, 1, input);
        }
        else
        {
            const Shot& shot{ m_File.Shots[m_Playlist[*position]] };

            // A POV agent that left the table ends its shot. Without a Tick nothing is known, so nothing ends.
            const bool isAgentGone{ shot.Kind == ShotKind::Pov && input.HasRoster &&
                !std::ranges::any_of(input.Roster, [this](const RosterEntry& entry) { return entry.Handle == m_PovHandle; }) };

            const double elapsed{ (std::max)(0.0, duration<double>(input.Now - m_ShotStart).count()) };
            m_RoundChanges = AddRoundChange(m_RoundChanges, isRoundChanged, elapsed);

            const std::optional<std::size_t> next{ Advance(AdvanceInput{ .Count = count, .Index = *position, .ElapsedSeconds = elapsed, .End = shot.End,
                .RoundChanges = m_RoundChanges }) };

            if (next || isAgentGone) this->Begin(next.value_or((*position + 1) % count), 1, input);
        }

        return this->Publish(input);
    }

    auto DirectorService::Publish(const DirectorInput& input) -> DirectorFrame
    {
        const std::size_t count{ m_Playlist.size() };
        const bool isShotThere{ !m_IsHolding && m_Index < count && m_Playlist[m_Index] < m_File.Shots.size() };
        const Shot* shot{ isShotThere ? &m_File.Shots[m_Playlist[m_Index]] : nullptr };

        DirectorStatus status{ .IsPlaying = true, .IsHolding = true, .Index = m_Index, .Count = count, .SequenceName = m_File.ActiveSequence };

        if (!shot || !IsShotValid(*shot))
        {
            m_Store.SetStatus(std::move(status));
            return DirectorFrame{ .IsPlaying = true };
        }

        const double elapsed{ (std::max)(0.0, duration<double>(input.Now - m_ShotStart).count()) };

        DirectorFrame frame{ .IsPlaying = true, .HasShot = true, .Kind = shot->Kind };

        if (shot->Kind == ShotKind::Fixed)
        {
            frame.Eye = shot->Eye;
            frame.Yaw = shot->Yaw;
            frame.Pitch = shot->Pitch;
        }
        else if (shot->Kind == ShotKind::Orbit)
        {
            const ShotPose pose{ OrbitPose(*shot, elapsed) };
            const ShotYawPitch angles{ YawPitchOf(pose.Forward) };

            frame.Eye = pose.Eye;
            frame.Yaw = angles.Yaw;
            frame.Pitch = angles.Pitch;
        }
        else
        {
            frame.PovPlayer = m_PovHandle;
        }

        status.IsHolding = false;
        status.ElapsedSeconds = static_cast<float>(elapsed);
        status.RoundChanges = m_RoundChanges;
        status.ShotId = shot->Id;
        status.Kind = shot->Kind;
        status.End = shot->End;
        status.ShotName = shot->Name;
        status.PovPlayer = frame.PovPlayer;

        m_Store.SetStatus(std::move(status));
        return frame;
    }
}