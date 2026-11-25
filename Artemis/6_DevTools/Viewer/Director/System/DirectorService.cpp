module DevTools.Viewer.Director.System;
import :Shots;

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
    using DevTools::Viewer::Director::Type::ParsedShots;
    using DevTools::Viewer::Director::Type::RoundKey;
    using DevTools::Viewer::Director::Type::Shot;
    using DevTools::Viewer::Director::Type::ShotKind;
    using DevTools::Viewer::Director::Type::ShotList;
    using DevTools::Viewer::Director::Type::ShotPose;
    using DevTools::Viewer::Director::Type::ShotYawPitch;
    using DevTools::Viewer::Roster::Type::RosterEntry;

    using Common::Engine::Type::k_NoHandle;
    using DevTools::Viewer::Director::Type::k_RetrySeconds;

    constexpr std::string_view k_DirectorFolder{ "Director" };
    constexpr std::string_view k_FileExtension{ ".txt" };

    // return: The file of the map inside the Artemis folder.
    auto FileOf(const std::string& directory, const std::string& key) -> std::filesystem::path
    {
        return std::filesystem::path{ directory } / k_DirectorFolder / (key + std::string{ k_FileExtension });
    }

    // return: The words for the panel after a load.
    auto LoadedMessage(std::size_t shots, std::size_t skipped) -> std::string
    {
        std::string message{ std::format("loaded {} shot(s)", shots) };
        if (skipped > 0) message += std::format(", {} invalid line(s) skipped", skipped);
        return message;
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

        if (directory.empty())
        {
            m_Store.SetShots(ShotList{});
            info.Message = "no Artemis folder (AppData is off): the shots are not saved";
        }
        else
        {
            const std::filesystem::path path{ FileOf(directory, key) };
            info.Path = path.string();

            std::ifstream file{ path, std::ios::binary };
            if (!file)
            {
                m_Store.SetShots(ShotList{});
                info.Message = "no file yet";
            }
            else
            {
                const std::string text{ std::istreambuf_iterator<char>{ file }, std::istreambuf_iterator<char>{} };
                ParsedShots parsed{ ParseShots(text) };

                info.Message = LoadedMessage(parsed.Shots.size(), parsed.Skipped);
                m_Store.SetShots(std::move(parsed.Shots));
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

        const ShotList shots{ m_Store.CopyShots() };
        const std::filesystem::path path{ FileOf(directory, key) };
        info.Path = path.string();

        std::error_code error{};
        std::filesystem::create_directories(path.parent_path(), error);

        // note: Written aside and renamed over the file, so a crash never leaves half a list.
        std::filesystem::path temporary{ path };
        temporary += ".tmp";

        bool isWritten{ false };
        if (!error)
        {
            std::ofstream file{ temporary, std::ios::binary | std::ios::trunc };
            file << FormatShots(shots);
            file.flush();
            isWritten = static_cast<bool>(file);
        }

        if (isWritten) std::filesystem::rename(temporary, path, error);

        info.Message = isWritten && !error ? std::format("saved {} shot(s)", shots.size()) : "save failed";
        m_Store.SetFileInfo(std::move(info));
    }

    auto DirectorService::Stop() -> void
    {
        m_Store.SetPlaying(false);
        m_Store.ClearRequests();
        m_Store.SetStatus(DirectorStatus{});
        m_IsRunning = false;
        m_IsHolding = false;
    }

    auto DirectorService::Begin(const ShotList& shots, std::size_t first, int direction, const DirectorInput& input) -> void
    {
        const std::optional<std::uint32_t> agent{ input.HasRoster ? PickPovAgent(input.Roster, m_LastPovAgent) : std::nullopt };
        const std::optional<std::size_t> index{ NextPlayableShot(shots, first, direction, agent.has_value()) };

        m_PovHandle = k_NoHandle;

        if (!index)
        {
            m_IsHolding = true;
            m_RetryAt = input.Now + std::chrono::duration_cast<steady_clock::duration>(duration<double>(k_RetrySeconds));
            return;
        }

        m_IsHolding = false;
        m_Index = *index;
        m_ShotStart = input.Now;

        if (shots[*index].Kind == ShotKind::Pov)
        {
            m_PovHandle = *agent;
            m_LastPovAgent = *agent;
        }
    }

    auto DirectorService::Start(const DirectorInput& input, const std::optional<RoundKey>& round) -> bool
    {
        const ShotList shots{ m_Store.CopyShots() };
        if (shots.empty()) return false;

        m_IsRunning = true;
        m_SeenLoadSerial = m_Store.GetLoadSerial();

        // The round the playback starts in is the baseline, so it is not a change.
        m_LastRound = round;

        this->Begin(shots, 0, 1, input);
        return true;
    }

    auto DirectorService::Update(const DirectorInput& input) -> DirectorFrame
    {
        if (m_Store.ConsumeTogglePlay()) m_Store.SetPlaying(!m_Store.IsPlaying());

        const int steps{ m_Store.ConsumeStep() };

        if (!m_Store.IsPlaying())
        {
            m_IsRunning = false;
            m_IsHolding = false;
            m_Store.SetStatus(DirectorStatus{});
            return DirectorFrame{};
        }

        const std::optional<RoundKey> round{ input.Round ? std::optional<RoundKey>{ RoundKey{ .MatchId = m_Store.GetMatchId(), .Round = *input.Round } } : std::nullopt };

        if (!m_IsRunning)
        {
            if (!this->Start(input, round))
            {
                m_Store.SetPlaying(false);
                m_Store.SetStatus(DirectorStatus{});
                return DirectorFrame{};
            }

            return this->Publish(input);
        }

        const bool isRoundChanged{ round.has_value() && IsRoundChange(m_LastRound, *round) };
        if (round) m_LastRound = round;

        const std::size_t count{ m_Store.GetShotCount() };

        if (m_Store.GetLoadSerial() != m_SeenLoadSerial)
        {
            // Another map: its own list, from the first shot.
            m_SeenLoadSerial = m_Store.GetLoadSerial();
            this->Begin(m_Store.CopyShots(), 0, 1, input);
        }
        else if (steps != 0)
        {
            const std::ptrdiff_t size{ static_cast<std::ptrdiff_t>(count) };
            const std::ptrdiff_t target{ size == 0 ? 0 : ((static_cast<std::ptrdiff_t>(m_Index) + steps) % size + size) % size };
            this->Begin(m_Store.CopyShots(), static_cast<std::size_t>(target), steps > 0 ? 1 : -1, input);
        }
        else if (m_IsHolding)
        {
            if (input.Now >= m_RetryAt) this->Begin(m_Store.CopyShots(), 0, 1, input);
        }
        else
        {
            const std::optional<Shot> shot{ m_Store.GetShot(m_Index) };

            if (!shot)
            {
                // The list shrank under the shot that was playing.
                this->Begin(m_Store.CopyShots(), 0, 1, input);
            }
            else
            {
                // A POV agent that left the table ends its shot. Without a Tick nothing is known, so nothing ends.
                const bool isAgentGone{ shot->Kind == ShotKind::Pov && input.HasRoster &&
                    !std::ranges::any_of(input.Roster, [this](const RosterEntry& entry) { return entry.Handle == m_PovHandle; }) };

                const double elapsed{ (std::max)(0.0, duration<double>(input.Now - m_ShotStart).count()) };
                const std::optional<std::size_t> next{ Advance(AdvanceInput{ .Count = count, .Index = m_Index, .ElapsedSeconds = elapsed,
                    .Seconds = shot->Seconds, .IsRoundChanged = isRoundChanged, .IsCutOnRound = input.IsCutOnRound }) };

                if (next || isAgentGone) this->Begin(m_Store.CopyShots(), next.value_or((m_Index + 1) % count), 1, input);
            }
        }

        return this->Publish(input);
    }

    auto DirectorService::Publish(const DirectorInput& input) -> DirectorFrame
    {
        const std::size_t count{ m_Store.GetShotCount() };
        const std::optional<Shot> shot{ m_IsHolding ? std::nullopt : m_Store.GetShot(m_Index) };

        if (!shot || !IsShotValid(*shot))
        {
            m_Store.SetStatus(DirectorStatus{ .IsPlaying = true, .IsHolding = true, .Index = m_Index, .Count = count });
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

        m_Store.SetStatus(DirectorStatus{ .IsPlaying = true, .Index = m_Index, .Count = count, .ElapsedSeconds = static_cast<float>(elapsed),
            .PovPlayer = frame.PovPlayer });

        return frame;
    }
}