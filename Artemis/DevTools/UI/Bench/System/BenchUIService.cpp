module;

#include <windows.h>
#include "External/imgui/imgui.h"

module UI.Bench.System;

import Common.Engine.Type;
import Common.Log.Type;
import Common.Semantic.Type;
import Common.Math.System;
import Common.Text.System;
import Service.Logs.Type;
import Platform.Host.Type;
import Platform.Input.Type;
import Environment.Players.Type;
import Environment.Events.Type;
import Export.Tick.Type;
import UI.Simulation.Type;
import UI.Simulation.System;
import std;

namespace
{
    using std::chrono::steady_clock;
    using std::chrono::system_clock;
    using std::chrono::duration;
    using std::chrono::milliseconds;
    using std::chrono::seconds;
    using std::filesystem::path;

    using Common::Math::System::Distance;
    using Common::Text::System::JsonEscape;
    using Common::Log::Type::LogLevel;
    using Platform::Input::Type::InputState;
    using Platform::Input::Type::LocalInputSource;
    using Platform::Input::Type::PlayerActions;
    using UI::Simulation::Type::LocalPlayerAction;
    using UI::Simulation::System::BuildActionOverride;
    using Service::Logs::Type::LogEntry;
    using Environment::Players::Type::Player;
    using Environment::Events::Type::GameEvent;
    using Common::Semantic::Type::EventKey::EventKeyLabel;
    using Common::Semantic::Type::EventKey::FindEventKey;
    using UI::Bench::Type::BenchAction;
    using UI::Bench::Type::BenchCheck;
    using UI::Bench::Type::BenchPlayers;
    using UI::Bench::Type::BenchPlayerSample;
    using UI::Bench::Type::BenchProbe;
    using UI::Bench::Type::BenchProbeState;
    using UI::Bench::Type::BenchRequest;
    using UI::Bench::Type::BenchCheckResult;
    using UI::Bench::Type::BenchMatch;
    using UI::Bench::Type::BenchMatchReport;
    using UI::Bench::Type::BenchPhase;
    using UI::Bench::Type::BenchRun;
    using UI::Bench::Type::BenchScenario;

    using Common::Engine::Type::k_NoHandle;
    using Common::Engine::Type::k_HandleIndexMask;
    using Platform::Host::Type::k_GameModuleName;
    using Platform::Host::Type::k_HostModuleName;
    using UI::Bench::Type::k_BenchFileName;

    // How long each phase may take before the match is given up. The launch includes the wait for the lobby (180 s in the hook).
    constexpr seconds k_LaunchTimeout{ 200 };
    constexpr seconds k_LoadTimeout{ 180 };
    constexpr seconds k_LeaveTimeout{ 60 };

    // Ceiling of a match without limits (the gametype ends it): a paused or stuck match is left anyway.
    constexpr seconds k_RunTimeout{ 3600 };

    // Time after START was pressed without an engine start before the launch is requested again (START can be ignored by MCC).
    constexpr seconds k_RelaunchAfter{ 45 };

    // Time between two writes of state.json. Tools/game.py takes a state older than 10 s as a dead bench.
    constexpr seconds k_StateInterval{ 1 };

    // Tries to replace a file of the exchange that Tools/game.py may hold open, and the wait between them.
    constexpr int k_RenameAttempts{ 5 };
    constexpr milliseconds k_RenameRetryDelay{ 20 };

    // Lines of the log kept per match in the report.
    constexpr std::size_t k_MaxLogLines{ 60 };

    // Rows a probe writes at most per match, and the largest value it reads.
    constexpr std::size_t k_MaxProbeRows{ 20000 };
    constexpr std::size_t k_MaxProbeSize{ 256 };

    // return: Whether the action or request reaches the player.
    auto IsSelected(const Player& player, BenchPlayers players) -> bool
    {
        switch (players)
        {
        case BenchPlayers::All: return true;
        case BenchPlayers::Bots: return !player.LocalIndex.has_value();
        case BenchPlayers::Locals: return player.LocalIndex.has_value();
        }
        return false;
    }

    auto ParsePlayers(std::string_view text) -> std::optional<BenchPlayers>
    {
        if (text == "all") return BenchPlayers::All;
        if (text == "bots") return BenchPlayers::Bots;
        if (text == "locals") return BenchPlayers::Locals;
        return std::nullopt;
    }

    auto ParseMode(std::string_view text) -> std::optional<LocalPlayerAction>
    {
        if (text == "engine") return LocalPlayerAction::Engine;
        if (text == "hold_still") return LocalPlayerAction::HoldStill;
        if (text == "test_action") return LocalPlayerAction::TestAction;
        if (text == "test_use") return LocalPlayerAction::TestUse;
        if (text == "hold_use") return LocalPlayerAction::HoldUse;
        return std::nullopt;
    }

    // return: The number in hex (with or without 0x), empty if the text is not one.
    auto ParseHex(std::string_view text) -> std::optional<std::uintptr_t>
    {
        if (text.starts_with("0x") || text.starts_with("0X")) text.remove_prefix(2);
        std::uintptr_t value{};
        const auto* end{ text.data() + text.size() };
        const auto result{ std::from_chars(text.data(), end, value, 16) };
        if (text.empty() || result.ec != std::errc{} || result.ptr != end) return std::nullopt;
        return value;
    }

    // Reads "haloreach+0x1234>0x10>0x8": the base, the first offset, then the offsets added after reading each pointer.
    // return: False if the text is not in that form.
    auto ParseProbeAddress(std::string_view text, BenchProbe& probe) -> bool
    {
        const std::size_t plus{ text.find('+') };
        if (plus == std::string_view::npos) return false;
        probe.Module = text.substr(0, plus);
        if (probe.Module != "haloreach" && probe.Module != "mcc" && probe.Module != "abs") return false;

        std::string_view rest{ text.substr(plus + 1) };
        while (!rest.empty())
        {
            const std::size_t next{ (std::min)(rest.find('>'), rest.size()) };
            const auto offset{ ParseHex(rest.substr(0, next)) };
            if (!offset.has_value()) return false;
            probe.Offsets.push_back(*offset);
            rest.remove_prefix((std::min)(next + 1, rest.size()));
        }
        return !probe.Offsets.empty();
    }

    // return: The bytes as hex, most significant byte first (how a little-endian value reads).
    auto ToHexValue(std::span<const std::byte> bytes) -> std::string
    {
        std::string text{};
        for (auto it{ bytes.rbegin() }; it != bytes.rend(); ++it) text += std::format("{:02X}", static_cast<unsigned>(*it));
        return text;
    }

    // return: Seconds from the time point to now.
    auto SecondsSince(steady_clock::time_point start) -> double
    {
        return duration<double>(steady_clock::now() - start).count();
    }

    // return: The current time in UTC, ISO 8601 with milliseconds (what Python's fromisoformat reads).
    auto UtcNow() -> std::string
    {
        return std::format("{:%FT%T}+00:00", std::chrono::floor<milliseconds>(system_clock::now()));
    }

    // return: The text without spaces, tabs and line ends at both sides.
    auto Trim(std::string_view text) -> std::string_view
    {
        const std::size_t first{ text.find_first_not_of(" \t\r\n") };
        if (first == std::string_view::npos) return {};
        const std::size_t last{ text.find_last_not_of(" \t\r\n") };
        return text.substr(first, last - first + 1);
    }

    // return: The number in the text, empty if the whole text is not one.
    template <typename T>
    auto ParseNumber(std::string_view text) -> std::optional<T>
    {
        T value{};
        const auto* end{ text.data() + text.size() };
        const auto result{ std::from_chars(text.data(), end, value) };
        if (result.ec != std::errc{} || result.ptr != end) return std::nullopt;
        return value;
    }

    // return: The words of the text, split by spaces.
    auto SplitWords(std::string_view text) -> std::vector<std::string>
    {
        std::vector<std::string> words{};
        std::size_t position{ 0 };
        while (position < text.size())
        {
            const std::size_t start{ text.find_first_not_of(' ', position) };
            if (start == std::string_view::npos) break;
            const std::size_t end{ (std::min)(text.find(' ', start), text.size()) };
            words.emplace_back(text.substr(start, end - start));
            position = end;
        }
        return words;
    }

    auto ToText(BenchPhase phase) -> std::string_view
    {
        switch (phase)
        {
        case BenchPhase::Idle: return "Idle";
        case BenchPhase::Launching: return "Launching";
        case BenchPhase::Loading: return "Loading";
        case BenchPhase::Running: return "Running";
        case BenchPhase::Leaving: return "Leaving";
        }
        return "Unknown";
    }

    // return: The list as a JSON array of strings.
    auto ToJson(const std::vector<std::string>& values) -> std::string
    {
        std::string text{ "[" };
        for (std::size_t i{ 0 }; i < values.size(); ++i)
        {
            if (i > 0) text += ',';
            text += std::format("\"{}\"", JsonEscape(values[i]));
        }
        return text + "]";
    }

    // return: Whether the match ended in a way that does not fail it by itself.
    auto IsCleanEnd(std::string_view end) -> bool
    {
        return end == "gametype" || end == "limit";
    }

    // return: Players of the tick, null without one.
    auto GetPlayers(const std::shared_ptr<const Export::Tick::Type::Tick>& tick) -> const std::vector<Player>*
    {
        return tick != nullptr && tick->Players != nullptr ? &tick->Players->Rows : nullptr;
    }
}

namespace UI::Bench::System
{
    auto BenchUIService::Initialize() -> void
    {
        const path file{ path{ m_SettingsStore.GetBaseDirectory() } / k_BenchFileName };
        std::error_code error{};

        if (!std::filesystem::exists(file, error)) return;

        std::ifstream input{ file };
        std::string text{ std::istreambuf_iterator<char>{ input }, std::istreambuf_iterator<char>{} };
        const path folder{ std::string{ Trim(text) } };

        if (folder.empty())
        {
            m_Log.Message("[BenchUIService] WARNING: The bench file is empty, the bench stays off.");
            return;
        }

        for (const char* name : { "inbox", "taken", "report", "probes", "events", "scenarios" })
        {
            std::filesystem::create_directories(folder / name, error);
            if (error) break;
        }

        if (error)
        {
            m_Log.Message("[BenchUIService] WARNING: Failed to create the folders of {} ({}), the bench stays off.",
                folder.string(), error.message());
            return;
        }

        m_BenchUIStore.SetFolder(folder);
        m_BenchUIStore.SetStatus("Waiting for a scenario.");
        m_Log.Message("[BenchUIService] INFO: Bench on, exchange folder {}.", folder.string());
    }

    auto BenchUIService::Update() -> void
    {
        if (!m_BenchUIStore.IsEnabled()) return;

        BenchRun& run{ m_BenchUIStore.GetRun() };

        // A failure of one step must not stop the heartbeat of state.json, or Tools/game.py takes the bench for dead.
        try
        {
            if (!run.Scenario.has_value()) this->TakeScenario(run);
            if (run.Scenario.has_value()) this->Step(run);
        }
        catch (const std::exception& exception)
        {
            if (run.Message != exception.what())
            {
                run.Message = exception.what();
                m_Log.Message("[BenchUIService] ERROR: Bench step failed ({}).", run.Message);
            }
        }

        if (steady_clock::now() - run.LastStateWrite >= k_StateInterval) this->WriteState(run);
    }

    auto BenchUIService::Draw() -> void
    {
        ImGui::SeparatorText("Test bench");

        if (!m_BenchUIStore.IsEnabled())
        {
            ImGui::TextDisabled("Off. The game watcher turns it on when it deploys the DLL (D129).");
            return;
        }

        // Draw runs inside the frame of the game: a file name it cannot convert must not take the game down.
        try
        {
            this->DrawScenarios();
        }
        catch (const std::exception& exception)
        {
            ImGui::TextDisabled("The scenarios cannot be listed (%s).", exception.what());
        }
    }

    auto BenchUIService::DrawScenarios() -> void
    {
        const std::string status{ m_BenchUIStore.GetStatus() };
        ImGui::TextWrapped("%s", status.c_str());

        const path folder{ m_BenchUIStore.GetFolder() / "scenarios" };
        std::vector<path>& files{ m_BenchUIStore.GetScenarioFiles() };
        const std::size_t selected{ m_BenchUIStore.GetSelectedScenario() };
        const std::string preview{ selected < files.size() ? files[selected].stem().string() : std::string{ "(none)" } };

        if (ImGui::BeginCombo("Scenario", preview.c_str()))
        {
            // Listed when the combo opens: Tools/game.py export can write the files while the game runs.
            files.clear();
            std::error_code error{};
            for (std::filesystem::directory_iterator it{ folder, error }; !error && it != std::filesystem::directory_iterator{};
                it.increment(error))
            {
                if (it->path().extension() == ".txt") files.push_back(it->path());
            }
            std::ranges::sort(files);

            for (std::size_t i{ 0 }; i < files.size(); ++i)
            {
                const std::string name{ files[i].stem().string() };
                if (ImGui::Selectable(name.c_str(), i == selected)) m_BenchUIStore.SetSelectedScenario(i);
            }

            ImGui::EndCombo();
        }

        ImGui::SameLine();
        if (ImGui::Button("Run") && selected < files.size()) this->QueueScenario(files[selected]);

        ImGui::TextDisabled("%s", folder.string().c_str());
    }

    auto BenchUIService::QueueScenario(const path& file) -> void
    {
        const auto now{ std::chrono::floor<milliseconds>(system_clock::now()) };
        const auto second{ std::chrono::floor<seconds>(now) };
        const path target{ m_BenchUIStore.GetFolder() / "inbox" / std::format("{:%Y%m%d-%H%M%S}-ui{:03}.txt", second,
            (now - second).count()) };
        const path temporary{ path{ target }.concat(".tmp") };
        std::error_code error{};

        // Copied under another extension first: the bench thread only takes .txt files, never a half written one.
        std::filesystem::copy_file(file, temporary, std::filesystem::copy_options::overwrite_existing, error);
        if (!error) std::filesystem::rename(temporary, target, error);

        if (error)
        {
            m_Log.Message("[BenchUIService] WARNING: Failed to queue the scenario {} ({}).", file.stem().string(),
                error.message());
            return;
        }

        m_Log.Message("[BenchUIService] INFO: Scenario {} queued from the Simulation tab.", file.stem().string());
    }

    auto BenchUIService::TakeScenario(BenchRun& run) -> void
    {
        const path folder{ m_BenchUIStore.GetFolder() };
        std::error_code error{};
        std::optional<path> oldest{};

        for (const auto& entry : std::filesystem::directory_iterator{ folder / "inbox", error })
        {
            if (entry.path().extension() != ".txt") continue;
            if (!oldest.has_value() || entry.path().filename() < oldest->filename()) oldest = entry.path();
        }

        if (!oldest.has_value()) return;

        std::string text{};
        {
            std::ifstream input{ *oldest };
            text.assign(std::istreambuf_iterator<char>{ input }, std::istreambuf_iterator<char>{});
        }

        // Moved out before it runs: a scenario that crashes the game is not taken again after the restart. If the file cannot be
        // moved yet (the writer still holds it), it is taken on a later update.
        std::filesystem::rename(*oldest, folder / "taken" / oldest->filename(), error);
        if (error) return;

        BenchScenario scenario{};
        scenario.Id = oldest->stem().string();
        const std::string problem{ this->Parse(text, scenario) };

        run = BenchRun{};
        run.ScenarioStart = steady_clock::now();
        run.Scenario = std::move(scenario);

        for (std::size_t i{ 0 }; i < run.Scenario->Matches.size(); ++i)
        {
            const BenchMatch& match{ run.Scenario->Matches[i] };
            run.Reports.push_back(BenchMatchReport{ .Index = i + 1, .Name = match.Name, .Config = match.Config });
        }

        if (!problem.empty() || run.Scenario->Matches.empty())
        {
            run.Message = problem.empty() ? "The scenario has no matches." : problem;
            m_Log.Message("[BenchUIService] WARNING: Scenario {} rejected: {}", run.Scenario->Id, run.Message);
            this->WriteReport(run, true);
            run = BenchRun{};
            return;
        }

        m_Log.Message("[BenchUIService] INFO: Scenario {} ({}) started, {} matches.", run.Scenario->Id, run.Scenario->Name,
            run.Scenario->Matches.size());
        this->WriteReport(run, false);
    }

    auto BenchUIService::Parse(std::string_view text, BenchScenario& scenario) const -> std::string
    {
        std::size_t lineNumber{ 0 };

        while (!text.empty())
        {
            const std::size_t lineEnd{ (std::min)(text.find('\n'), text.size()) };
            const std::string_view line{ Trim(text.substr(0, lineEnd)) };
            text.remove_prefix((std::min)(lineEnd + 1, text.size()));
            ++lineNumber;

            if (line.empty() || line.front() == '#') continue;

            const std::size_t space{ (std::min)(line.find(' '), line.size()) };
            const std::string_view key{ line.substr(0, space) };
            const std::string_view value{ Trim(line.substr(space)) };
            const auto fail{ [&](std::string_view why)
            {
                return std::format("line {}: {} ({}).", lineNumber, why, line);
            } };

            if (key == "name")
            {
                scenario.Name = value;
                continue;
            }

            if (key == "match")
            {
                scenario.Matches.push_back(BenchMatch{ .Name = std::string{ value } });
                continue;
            }

            if (scenario.Matches.empty()) return fail("key before the first match");

            BenchMatch& match{ scenario.Matches.back() };

            if (key == "config")
            {
                match.Config = value;
            }
            else if (key == "locals")
            {
                const auto count{ ParseNumber<std::size_t>(value) };
                if (!count.has_value()) return fail("locals takes a number");
                match.Locals = count;
            }
            else if (key == "bot")
            {
                if (value == "auto")
                {
                    match.Bots.push_back(std::nullopt);
                    continue;
                }
                const auto team{ ParseNumber<int>(value) };
                if (!team.has_value() || *team < 0 || *team > 7) return fail("bot takes auto or a team 0-7");
                match.Bots.push_back(static_cast<std::int8_t>(*team));
            }
            else if (key == "scale")
            {
                const auto scale{ ParseNumber<float>(value) };
                if (!scale.has_value() || *scale < 0.0f) return fail("scale takes a number >= 0");
                match.TimeScale = scale;
            }
            else if (key == "max_ticks")
            {
                const auto ticks{ ParseNumber<std::uint64_t>(value) };
                if (!ticks.has_value()) return fail("max_ticks takes a number");
                match.MaxTicks = *ticks;
            }
            else if (key == "max_seconds")
            {
                const auto limit{ ParseNumber<double>(value) };
                if (!limit.has_value()) return fail("max_seconds takes a number");
                match.MaxSeconds = *limit;
            }
            else if (key == "action")
            {
                const std::vector<std::string> words{ SplitWords(value) };
                const auto players{ words.size() >= 2 ? ParsePlayers(words[0]) : std::nullopt };
                const auto mode{ words.size() >= 2 ? ParseMode(words[1]) : std::nullopt };
                const auto from{ words.size() >= 3 ? ParseNumber<std::uint64_t>(words[2]) : std::optional<std::uint64_t>{ 0 } };
                const auto to{ words.size() >= 4 ? ParseNumber<std::uint64_t>(words[3]) : std::optional<std::uint64_t>{ 0 } };
                if (!players || !mode || !from || !to) return fail("action takes all|bots|locals, a mode and optional from and to ticks");
                match.Actions.push_back(BenchAction{ .Players = *players, .Mode = *mode, .FromTick = *from, .ToTick = *to });
            }
            else if (key == "request")
            {
                const std::vector<std::string> words{ SplitWords(value) };
                const auto players{ words.size() == 3 ? ParsePlayers(words[0]) : std::nullopt };
                const auto at{ words.size() == 3 ? ParseNumber<std::uint64_t>(words[2]) : std::nullopt };
                BenchRequest request{ .AtTick = at.value_or(0) };
                request.Requests.IsWeaponSwitch = words.size() == 3 && words[1] == "weapon";
                request.Requests.IsGrenadeSwitch = words.size() == 3 && words[1] == "grenade";
                request.Requests.IsZoom = words.size() == 3 && words[1] == "zoom";
                const bool isKnown{ request.Requests.IsWeaponSwitch || request.Requests.IsGrenadeSwitch || request.Requests.IsZoom };
                if (!players || !at || !isKnown) return fail("request takes all|bots|locals, weapon|grenade|zoom and a tick");
                request.Players = *players;
                match.Requests.push_back(request);
            }
            else if (key == "probe")
            {
                const std::vector<std::string> words{ SplitWords(value) };
                BenchProbe probe{};
                const auto size{ words.size() == 3 ? ParseNumber<std::size_t>(words[2]) : std::nullopt };
                if (!size || *size == 0 || *size > k_MaxProbeSize || !ParseProbeAddress(words[1], probe))
                {
                    return fail("probe takes a name, an address like haloreach+0x1234>0x10 and a size of 1-256 bytes");
                }
                probe.Name = words[0];
                probe.Size = *size;
                match.Probes.push_back(std::move(probe));
            }
            else if (key == "ticktrace")
            {
                match.IsTickTrace = true;
            }
            else if (key == "events")
            {
                match.IsEvents = true;
            }
            else if (key == "check")
            {
                std::vector<std::string> words{ SplitWords(value) };
                if (words.empty()) return fail("check takes a name");
                BenchCheck check{ .Name = words.front() };
                check.Arguments.assign(words.begin() + 1, words.end());
                match.Checks.push_back(std::move(check));
            }
            else
            {
                return fail("unknown key");
            }
        }

        for (const BenchMatch& match : scenario.Matches)
        {
            if (match.Config.empty()) return std::format("match {} has no config.", match.Name);
        }

        return {};
    }

    auto BenchUIService::BeginMatch(BenchRun& run) -> void
    {
        const BenchMatch& match{ run.Scenario->Matches[run.MatchIndex] };
        BenchMatchReport& report{ run.Reports[run.MatchIndex] };
        std::vector<LogEntry> ignored{};

        run.LogSequence = m_LogsStore.CopySince((std::numeric_limits<std::uint64_t>::max)(), ignored);
        run.LastTick = nullptr;
        run.PhaseStart = steady_clock::now();
        report.Result = "running";

        m_Log.Message("[BenchUIService] INFO: Match {} of {} ({}, config {}).", report.Index, run.Reports.size(), match.Name,
            match.Config);

        if (!m_MatchConfigService.LoadPending(match.Config))
        {
            report.End = "no-config";
            this->EndMatch(run);
            return;
        }

        if (match.Locals.has_value())
        {
            m_LocalUserStore.SetRequestedCount(*match.Locals);

            // Without a real gamepad, Device pauses the game for an Artemis user (gamepad disconnected, black screen), so the users
            // the bench adds play an idle virtual gamepad, as the Simulation tab does. User 0 is the person and keeps the devices.
            // The Simulation tab still lists them as Device: its choice is its own, the source is what the input hook reads.
            for (std::size_t user{ 1 }; user < *match.Locals; ++user)
            {
                if (m_VirtualInputStore.GetSource(user) != LocalInputSource::Device) continue;

                m_VirtualInputStore.SetState(user, InputState{});
                m_VirtualInputStore.SetSource(user, LocalInputSource::Virtual);
            }
        }

        const auto tick{ m_TickStore.Acquire() };
        run.GenerationBefore = tick != nullptr ? tick->Generation : 0;
        run.EngineStarts = m_MatchCycleStore.GetEngineStarts();
        run.EngineDestroys = m_MatchCycleStore.GetEngineDestroys();
        run.Phase = BenchPhase::Launching;
        run.LaunchRequest = steady_clock::now();

        m_MatchConfigService.RequestLaunch();
    }

    auto BenchUIService::Step(BenchRun& run) -> void
    {
        if (run.Phase == BenchPhase::Idle)
        {
            this->BeginMatch(run);
            return;
        }

        const BenchMatch& match{ run.Scenario->Matches[run.MatchIndex] };
        BenchMatchReport& report{ run.Reports[run.MatchIndex] };
        const double elapsed{ SecondsSince(run.PhaseStart) };
        const bool isDestroyed{ m_MatchCycleStore.GetEngineDestroys() > run.EngineDestroys };

        switch (run.Phase)
        {
        case BenchPhase::Launching:
            if (m_MatchCycleStore.GetEngineStarts() > run.EngineStarts)
            {
                report.LaunchSeconds = elapsed;
                run.Phase = BenchPhase::Loading;
                run.PhaseStart = steady_clock::now();

                // A relaunch still pending would press START in the next lobby. A start means the engine before it is gone, so
                // its late destruction and its last ticks belong to the previous match.
                m_MatchConfigService.CancelLaunch();
                run.EngineDestroys = m_MatchCycleStore.GetEngineDestroys();
                const auto last{ m_TickStore.Acquire() };
                if (last != nullptr) run.GenerationBefore = last->Generation;
            }
            else if (elapsed > static_cast<double>(k_LaunchTimeout.count()))
            {
                m_MatchConfigService.CancelLaunch();
                report.End = "launch-timeout";
                this->EndMatch(run);
            }
            else if (!m_MatchConfigStore.IsLaunchRequested() && steady_clock::now() - run.LaunchRequest > k_RelaunchAfter)
            {
                m_Log.Message("[BenchUIService] WARNING: No engine start {} s after the launch (MCC ignored START or showed a dialog), "
                    "requesting it again.",
                    k_RelaunchAfter.count());
                run.LaunchRequest = steady_clock::now();
                m_MatchConfigService.RequestLaunch();
            }
            break;

        case BenchPhase::Loading:
        {
            const auto tick{ m_TickStore.Acquire() };

            if (isDestroyed)
            {
                report.End = "destroyed-while-loading";
                this->EndMatch(run);
            }
            else if (tick != nullptr && tick->Generation != run.GenerationBefore)
            {
                report.LoadSeconds = elapsed;
                run.FirstGeneration = tick->Generation;
                run.LastGeneration = tick->Generation;
                run.LastTick = tick;
                run.Phase = BenchPhase::Running;
                run.PhaseStart = steady_clock::now();

                // Bots and teams only exist after the start (D110). The speed is set when the match runs, so loading is not scaled.
                for (const auto& team : match.Bots) m_BotStore.RequestBot(team);
                if (match.TimeScale.has_value()) m_LifecycleStore.SetRequestedTimeScale(*match.TimeScale);

                this->StartTools(run);
                this->Sample(run, 1);
            }
            else if (elapsed > static_cast<double>(k_LoadTimeout.count()))
            {
                this->Leave(run, "load-timeout");
            }
            break;
        }

        case BenchPhase::Running:
        {
            const auto tick{ m_TickStore.Acquire() };

            if (tick != nullptr && tick->Generation != run.LastGeneration)
            {
                run.LastGeneration = tick->Generation;
                run.LastTick = tick;
                report.Ticks = run.LastGeneration - run.FirstGeneration + 1;
                this->Sample(run, report.Ticks);
            }

            if (isDestroyed)
            {
                report.RunSeconds = elapsed;
                report.End = "gametype";
                this->EndMatch(run);
            }
            else if ((match.MaxTicks > 0 && report.Ticks >= match.MaxTicks) || (match.MaxSeconds > 0.0 && elapsed >= match.MaxSeconds))
            {
                report.RunSeconds = elapsed;
                this->Leave(run, "limit");
            }
            else if (elapsed >= static_cast<double>(k_RunTimeout.count()))
            {
                report.RunSeconds = elapsed;
                this->Leave(run, "run-timeout");
            }
            break;
        }

        case BenchPhase::Leaving:
            if (isDestroyed)
            {
                this->EndMatch(run);
            }
            else if (elapsed > static_cast<double>(k_LeaveTimeout.count()))
            {
                report.End = "leave-timeout";
                this->EndMatch(run);
            }
            break;

        case BenchPhase::Idle:
            break;
        }
    }

    auto BenchUIService::StartTools(BenchRun& run) -> void
    {
        const BenchMatch& match{ run.Scenario->Matches[run.MatchIndex] };
        BenchMatchReport& report{ run.Reports[run.MatchIndex] };

        run.SentRequests.assign(match.Requests.size(), false);
        run.Probes.clear();

        for (const BenchProbe& probe : match.Probes)
        {
            BenchProbeState state{ .File = m_BenchUIStore.GetFolder() / "probes" / std::format("{}_m{}_{}.csv", run.Scenario->Id,
                report.Index, probe.Name) };
            std::ofstream output{ state.File, std::ios::trunc };
            output << "tick,seconds,address,value\n";
            report.Files.emplace_back("probe " + probe.Name, state.File.string());
            run.Probes.push_back(std::move(state));
        }

        run.EventCursor = 0;
        if (match.IsEvents)
        {
            run.EventsFile = m_BenchUIStore.GetFolder() / "events" / std::format("{}_m{}.csv", run.Scenario->Id, report.Index);
            std::ofstream output{ run.EventsFile, std::ios::trunc };
            output << "sequence,game_tick,seconds,key,name,kind,cause_player,cause_object,effect_player,effect_object,drt,modifier,extra,data\n";
            report.Files.emplace_back("events", run.EventsFile.string());
        }

        if (match.IsTickTrace) m_TickTraceUIStore.RequestStart();
    }

    auto BenchUIService::Sample(BenchRun& run, std::uint64_t ticks) -> void
    {
        const BenchMatch& match{ run.Scenario->Matches[run.MatchIndex] };
        BenchMatchReport& report{ run.Reports[run.MatchIndex] };
        const auto* players{ GetPlayers(run.LastTick) };

        if (match.IsTickTrace && !run.IsTickTraceNoted && m_TickTraceUIStore.IsRecording())
        {
            run.IsTickTraceNoted = true;
            report.Files.emplace_back("ticktrace", m_TickTraceUIStore.GetFilePath().string());
        }

        if (players != nullptr)
        {
            PlayerActions latest{};
            bool isLatestCopied{ false };

            for (const Player& player : *players)
            {
                if (player.IsAlive())
                {
                    auto [sample, isNew]{ run.FirstSamples.try_emplace(player.Handle, BenchPlayerSample{
                        .Position = player.CameraPosition, .Weapon = player.Body->Loadout.EquippedWeapon }) };
                    if (!isNew)
                    {
                        sample->second.FarthestDistance = (std::max)(sample->second.FarthestDistance,
                            Distance(sample->second.Position, player.CameraPosition));
                    }
                    if (player.Body->Loadout.Objective != k_NoHandle) run.IsObjectiveSeen = true;
                }

                const std::size_t index{ player.Handle & k_HandleIndexMask };
                if (index >= latest.Actions.size()) continue;

                // The last action of the scenario whose window holds the tick wins. Outside every window the engine plays.
                LocalPlayerAction desired{ LocalPlayerAction::Engine };
                for (const BenchAction& action : match.Actions)
                {
                    const bool isInside{ ticks >= action.FromTick && (action.ToTick == 0 || ticks <= action.ToTick) };
                    if (isInside && IsSelected(player, action.Players)) desired = action.Mode;
                }

                const auto applied{ run.AppliedModes.find(index) };
                const LocalPlayerAction current{ applied != run.AppliedModes.end() ? applied->second : LocalPlayerAction::Engine };

                if (desired != current)
                {
                    if (!isLatestCopied)
                    {
                        m_PlayerActionStore.CopyLatest(latest);
                        isLatestCopied = true;
                    }

                    const std::optional<std::size_t> localPlayer{ player.LocalIndex.has_value()
                        ? std::optional<std::size_t>{ *player.LocalIndex } : std::nullopt };
                    m_PlayerActionStore.SetOverride(index, BuildActionOverride(desired, latest.Actions[index], localPlayer));
                    run.AppliedModes[index] = desired;
                }

                for (std::size_t i{ 0 }; i < match.Requests.size(); ++i)
                {
                    const BenchRequest& request{ match.Requests[i] };
                    if (!run.SentRequests[i] && ticks >= request.AtTick && IsSelected(player, request.Players))
                    {
                        m_PlayerActionStore.RequestClient(index, request.Requests);
                    }
                }
            }

            for (std::size_t i{ 0 }; i < match.Requests.size(); ++i)
            {
                if (ticks >= match.Requests[i].AtTick) run.SentRequests[i] = true;
            }
        }

        for (std::size_t i{ 0 }; i < match.Probes.size(); ++i)
        {
            const BenchProbe& probe{ match.Probes[i] };
            BenchProbeState& state{ run.Probes[i] };
            const auto address{ this->ResolveProbe(probe) };
            std::vector<std::byte> bytes(probe.Size);
            const bool isReadable{ address.has_value() && m_MemoryReaderService.ReadRaw(*address, bytes.data(), bytes.size()) };

            if (state.Rows > 0 && isReadable == state.IsReadable && (!isReadable || bytes == state.Last)) continue;
            if (state.Rows >= k_MaxProbeRows) continue;

            std::ofstream output{ state.File, std::ios::app };
            output << std::format("{},{:.3f},{:016X},{}\n", ticks, SecondsSince(run.PhaseStart), address.value_or(0),
                isReadable ? ToHexValue(bytes) : "unreadable");
            state.Last = std::move(bytes);
            state.IsReadable = isReadable;
            ++state.Rows;
        }

        const auto& events{ run.LastTick ? run.LastTick->Events : nullptr };
        if (match.IsEvents && events && events->LastSequence > run.EventCursor)
        {
            std::ofstream output{ run.EventsFile, std::ios::app };
            for (const GameEvent& event : events->Rows)
            {
                if (event.Sequence <= run.EventCursor) continue;

                const EventKeyLabel* label{ FindEventKey(event.Key) };
                output << std::format("{},{},{:.3f},{:06X},{},{},{:08X},{:08X},{:08X},{:08X},{},{},{:08X},{}\n", event.Sequence,
                    event.GameTick, SecondsSince(run.PhaseStart), event.Key, label ? label->Name : std::string_view{},
                    NameOf(event.Kind), event.CausePlayer, event.CauseObject, event.EffectPlayer, event.EffectObject,
                    event.DamageReportingType, event.Modifier, event.Extra, ToHexValue(std::as_bytes(std::span{ event.Data })));
            }
            run.EventCursor = events->LastSequence;
        }
    }

    auto BenchUIService::StopTools(BenchRun& run) -> void
    {
        for (const auto& [index, mode] : run.AppliedModes)
        {
            if (mode != LocalPlayerAction::Engine) m_PlayerActionStore.SetOverride(index, std::nullopt);
        }

        run.AppliedModes.clear();
        run.FirstSamples.clear();
        run.IsObjectiveSeen = false;
        run.IsTickTraceNoted = false;

        if (run.Scenario->Matches[run.MatchIndex].IsTickTrace) m_TickTraceUIStore.RequestStop();
    }

    auto BenchUIService::ResolveProbe(const BenchProbe& probe) -> std::optional<std::uintptr_t>
    {
        std::uintptr_t address{ 0 };
        if (probe.Module == "haloreach") address = reinterpret_cast<std::uintptr_t>(GetModuleHandleW(k_GameModuleName));
        if (probe.Module == "mcc") address = reinterpret_cast<std::uintptr_t>(GetModuleHandleW(k_HostModuleName));
        if (probe.Module != "abs" && address == 0) return std::nullopt;

        address += probe.Offsets.front();

        for (std::size_t i{ 1 }; i < probe.Offsets.size(); ++i)
        {
            std::uintptr_t pointer{ 0 };
            if (!m_MemoryReaderService.ReadRaw(address, &pointer, sizeof(pointer)) || pointer == 0) return std::nullopt;
            address = pointer + probe.Offsets[i];
        }

        return address;
    }

    auto BenchUIService::Leave(BenchRun& run, std::string_view end) -> void
    {
        run.Reports[run.MatchIndex].End = end;
        run.Phase = BenchPhase::Leaving;
        run.PhaseStart = steady_clock::now();

        m_MatchCycleStore.RequestLeave();
        m_Log.Message("[BenchUIService] INFO: Leaving match {} ({}).", run.MatchIndex + 1, end);
    }

    auto BenchUIService::EndMatch(BenchRun& run) -> void
    {
        const BenchMatch& match{ run.Scenario->Matches[run.MatchIndex] };
        BenchMatchReport& report{ run.Reports[run.MatchIndex] };

        m_MatchConfigService.CancelLaunch();

        // Back to the engine speed between matches, so the lobby and the next load run at normal speed.
        if (match.TimeScale.has_value()) m_LifecycleStore.SetRequestedTimeScale(0.0f);

        std::vector<LogEntry> entries{};
        m_LogsStore.CopySince(run.LogSequence, entries);

        for (const LogEntry& entry : entries)
        {
            if (entry.Level != LogLevel::Warning && entry.Level != LogLevel::Error) continue;
            if (report.Log.size() < k_MaxLogLines) report.Log.push_back(entry.FullText);
        }

        this->Check(run, report);
        this->StopTools(run);

        const bool isOk{ IsCleanEnd(report.End) && std::ranges::all_of(report.Checks, [](const BenchCheckResult& check) {
            return check.IsOk;
            }) };
        report.Result = isOk ? "ok" : "fail";

        m_Log.Message("[BenchUIService] INFO: Match {} ended ({}): {}, {} ticks.", report.Index, report.End, report.Result,
            report.Ticks);

        run.Phase = BenchPhase::Idle;
        ++run.MatchIndex;

        if (run.MatchIndex < run.Reports.size())
        {
            this->WriteReport(run, false);
            return;
        }

        this->WriteReport(run, true);
        m_Log.Message("[BenchUIService] INFO: Scenario {} finished.", run.Scenario->Id);
        run.Scenario.reset();
        run.Reports.clear();
        run.MatchIndex = 0;
    }

    auto BenchUIService::Check(const BenchRun& run, BenchMatchReport& report) const -> void
    {
        const BenchMatch& match{ run.Scenario->Matches[run.MatchIndex] };
        const auto* players{ GetPlayers(run.LastTick) };

        for (const BenchCheck& check : match.Checks)
        {
            BenchCheckResult result{ .Name = check.Name };
            const auto& arguments{ check.Arguments };
            const auto number{ [&](std::size_t index) -> std::optional<long long>
            {
                return index < arguments.size() ? ParseNumber<long long>(arguments[index]) : std::nullopt;
            } };

            if (check.Name == "no_errors")
            {
                const auto errors{ std::ranges::count_if(report.Log, [](const std::string& line) {
                    return line.find("ERROR") != std::string::npos;
                    }) };
                result.IsOk = errors == 0;
                result.Detail = std::format("{} ERROR lines", errors);
            }
            else if (check.Name == "ended_by" && !arguments.empty())
            {
                result.IsOk = report.End == arguments.front();
                result.Detail = std::format("ended by {}", report.End);
            }
            else if (check.Name == "min_ticks" && number(0).has_value())
            {
                result.IsOk = report.Ticks >= static_cast<std::uint64_t>(*number(0));
                result.Detail = std::format("{} ticks", report.Ticks);
            }
            else if ((check.Name == "players" || check.Name == "min_players") && number(0).has_value())
            {
                const long long count{ players != nullptr ? static_cast<long long>(players->size()) : -1 };
                result.IsOk = check.Name == "players" ? count == *number(0) : count >= *number(0);
                result.Detail = std::format("{} players on the last tick", count);
            }
            else if (check.Name == "team_players" && number(0).has_value() && number(1).has_value())
            {
                const long long count{ players == nullptr ? -1 : static_cast<long long>(std::ranges::count_if(*players,
                    [&](const Player& player) { return static_cast<long long>(player.Team) == *number(0); })) };
                result.IsOk = count == *number(1);
                result.Detail = std::format("{} players in team {}", count, *number(0));
            }
            else if (check.Name == "alive" && arguments.size() == 2 && ParsePlayers(arguments[0]) && number(1).has_value())
            {
                const auto selection{ *ParsePlayers(arguments[0]) };
                const long long count{ players == nullptr ? -1 : static_cast<long long>(std::ranges::count_if(*players,
                    [&](const Player& player) { return IsSelected(player, selection) && player.IsAlive(); })) };
                result.IsOk = count >= *number(1);
                result.Detail = std::format("{} alive on the last tick", count);
            }
            else if ((check.Name == "moved" || check.Name == "weapon_changed") && !arguments.empty() && ParsePlayers(arguments[0])
                && (check.Name == "weapon_changed" || (arguments.size() == 2 && ParseNumber<float>(arguments[1]).has_value())))
            {
                // Every selected player alive on the last tick, against the first tick it was seen alive (moved: the farthest it got).
                const auto selection{ *ParsePlayers(arguments[0]) };
                const float minimum{ check.Name == "moved" ? *ParseNumber<float>(arguments[1]) : 0.0f };
                std::size_t compared{ 0 };
                std::size_t passed{ 0 };
                float least{ (std::numeric_limits<float>::max)() };

                for (std::size_t i{ 0 }; players != nullptr && i < players->size(); ++i)
                {
                    const Player& player{ (*players)[i] };
                    const auto first{ run.FirstSamples.find(player.Handle) };
                    if (!IsSelected(player, selection) || !player.IsAlive() || first == run.FirstSamples.end()) continue;

                    ++compared;
                    const float moved{ (std::max)(first->second.FarthestDistance, Distance(first->second.Position, player.CameraPosition)) };
                    least = (std::min)(least, moved);
                    const bool isOk{ check.Name == "moved" ? moved >= minimum : player.Body->Loadout.EquippedWeapon != first->second.Weapon };
                    if (isOk) ++passed;
                }

                result.IsOk = compared > 0 && passed == compared;
                result.Detail = check.Name == "moved" ? std::format("{}/{} players moved >= {} (least {:.2f})", passed, compared, minimum,
                    compared > 0 ? least : 0.0f) : std::format("{}/{} players changed weapon", passed, compared);
            }
            else if (check.Name == "objective_taken")
            {
                result.IsOk = run.IsObjectiveSeen;
                result.Detail = run.IsObjectiveSeen ? "a player held the objective" : "no player held the objective on a sampled tick";
            }
            else
            {
                result.Detail = "unknown check or wrong arguments";
            }

            report.Checks.push_back(std::move(result));
        }
    }

    auto BenchUIService::WriteState(BenchRun& run) -> void
    {
        run.LastStateWrite = steady_clock::now();

        const bool hasScenario{ run.Scenario.has_value() };
        const std::size_t index{ hasScenario ? (std::min)(run.MatchIndex, run.Reports.size() - 1) : 0 };
        const BenchMatchReport* report{ hasScenario ? &run.Reports[index] : nullptr };
        const std::string status{ hasScenario ? std::format("{} match {}/{} {}", ToText(run.Phase), index + 1, run.Reports.size(),
            report->Name) : std::string{ "Waiting for a scenario." } };
        m_BenchUIStore.SetStatus(status);

        const std::string text{ std::format("{{\"heartbeat\":\"{}\",\"lobbyReady\":{},\"phase\":\"{}\",\"scenario\":\"{}\","
            "\"match\":\"{}\",\"matchIndex\":{},\"matchCount\":{},\"tick\":{},\"engineStarts\":{},\"engineDestroys\":{},"
            "\"lastLaunch\":\"{}\",\"lastLeave\":\"{}\",\"message\":\"{}\",\"configs\":{}}}",
            UtcNow(), m_MatchCycleStore.IsLobbyOpen(), hasScenario ? ToText(run.Phase) : "Waiting",
            hasScenario ? JsonEscape(run.Scenario->Id) : "", report != nullptr ? JsonEscape(report->Name) : "",
            hasScenario ? index + 1 : 0, run.Reports.size(), report != nullptr ? report->Ticks : 0,
            m_MatchCycleStore.GetEngineStarts(), m_MatchCycleStore.GetEngineDestroys(),
            JsonEscape(m_MatchConfigStore.GetLastLaunch()), JsonEscape(m_MatchCycleStore.GetLastLeave()), JsonEscape(status),
            ToJson(m_MatchConfigService.ListConfigs())) };

        this->WriteFile(m_BenchUIStore.GetFolder() / "state.json", text);
    }

    auto BenchUIService::WriteReport(const BenchRun& run, bool isDone) const -> void
    {
        std::string matches{};
        bool isOk{ run.Message.empty() };

        for (const BenchMatchReport& report : run.Reports)
        {
            std::string checks{};
            for (const BenchCheckResult& check : report.Checks)
            {
                if (!checks.empty()) checks += ',';
                checks += std::format("{{\"name\":\"{}\",\"ok\":{},\"detail\":\"{}\"}}", JsonEscape(check.Name), check.IsOk,
                    JsonEscape(check.Detail));
            }

            std::string files{};
            for (const auto& [name, file] : report.Files)
            {
                if (!files.empty()) files += ',';
                files += std::format("\"{}\":\"{}\"", JsonEscape(name), JsonEscape(file));
            }

            if (report.Result != "ok") isOk = false;
            if (!matches.empty()) matches += ',';
            matches += std::format("{{\"index\":{},\"name\":\"{}\",\"config\":\"{}\",\"result\":\"{}\",\"end\":\"{}\",\"ticks\":{},"
                "\"timing\":{{\"launch\":{:.1f},\"load\":{:.1f},\"run\":{:.1f}}},\"checks\":[{}],\"log\":{},\"files\":{{{}}}}}",
                report.Index, JsonEscape(report.Name), JsonEscape(report.Config), report.Result, JsonEscape(report.End), report.Ticks,
                report.LaunchSeconds, report.LoadSeconds, report.RunSeconds, checks, ToJson(report.Log), files);
        }

        const std::string result{ isDone ? (isOk ? "ok" : "fail") : "running" };
        const std::string text{ std::format("{{\"id\":\"{}\",\"scenario\":\"{}\",\"done\":{},\"result\":\"{}\",\"seconds\":{:.1f},"
            "\"message\":\"{}\",\"matches\":[{}]}}", JsonEscape(run.Scenario->Id), JsonEscape(run.Scenario->Name), isDone, result,
            SecondsSince(run.ScenarioStart), JsonEscape(run.Message), matches) };

        this->WriteFile(m_BenchUIStore.GetFolder() / "report" / (run.Scenario->Id + ".json"), text);
    }

    auto BenchUIService::WriteFile(const path& file, const std::string& text) const -> void
    {
        path temporary{ file };
        temporary += ".tmp";

        {
            std::ofstream output{ temporary, std::ios::binary | std::ios::trunc };
            if (!output) return;
            output << text;
        }

        // Tools/game.py may be reading the file: a few tries before the write is given up.
        std::error_code error{};
        for (int attempt{ 0 }; attempt < k_RenameAttempts; ++attempt)
        {
            std::filesystem::rename(temporary, file, error);
            if (!error) return;
            std::this_thread::sleep_for(k_RenameRetryDelay);
        }

        m_Log.Message("[BenchUIService] WARNING: Failed to write {} ({}).", file.filename().string(), error.message());
    }
}