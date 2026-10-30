#include <gtest/gtest.h>

#include <stdlib.h>

import Common.Hash.System;
import Common.Math.Type;
import Common.Semantic.Type;
import Common.Team.Type;
import Contract.Bridge.Type;
import Blam.Layout.Object.Type;
import Bridge.Codec.Dump.Type;
import Bridge.Codec.Dump.System;
import Bridge.Codec.Layer;
import Bridge.Codec.Tick.Type;
import Blam.Layout.Snapshot.Type;
import Blam.Layout.Snapshot.System;
import Blam.Map.Layer;
import Blam.MapModel.Layer;
import Perception.Details.Type;
import Perception.Events.Type;
import Perception.Match.Type;
import Tests.Support.Blam;
import Tests.Support.Maps;
import Tests.Support.Pipeline;
import Tests.Support.Replay;
import std;

// The first offline test of the core (M5c, D204): a .sdump replayed through the pipeline must give the records of the .tdump
// recorded in the same session, byte by byte. The synthetic recordings are written the way the DLL writes them. The real ones
// are the golden recordings of Ignore/Regresion.

namespace
{
    namespace SnapshotCodec = Blam::Layout::Snapshot::System;
    namespace TickCodec = Bridge::Codec::Dump::System;
    namespace ObjeOffset = Blam::Layout::Object::Type::Obje::Offset;

    using Common::Math::Type::Vec3;
    using Bridge::Codec::Dump::Type::TickDumpFileHeader;
    using Blam::Layout::Snapshot::Type::Snapshot;
    using Blam::Layout::Snapshot::Type::SnapshotDumpFooter;
    using Blam::Layout::Snapshot::Type::SnapshotLoadInfo;
    using Blam::Layout::Snapshot::Type::SnapshotReaderState;
    using Blam::Layout::Snapshot::Type::SnapshotReadStatus;
    using Blam::Layout::Snapshot::Type::SnapshotWriterState;
    using Contract::Bridge::Type::BridgeTick;
    using Tests::Support::Blam::Put;
    using Tests::Support::Maps::MapsFolder;
    using Tests::Support::Pipeline::Chain;
    using Tests::Support::Pipeline::FillSyntheticTags;
    using Tests::Support::Pipeline::WithBipedAndPlayer;
    using Tests::Support::Replay::Replay;
    using Tests::Support::Replay::ReplayResult;
    using Tests::Support::Replay::ReplayStatus;

    using Common::Hash::System::FileCrc32;
    using Blam::Layout::Snapshot::Type::k_SnapshotDumpEndMarker;
    using Blam::Layout::Snapshot::Type::k_SnapshotFirstAfterLoad;
    using Contract::Bridge::Type::k_BridgeLayoutHash;
    using Tests::Support::Pipeline::k_BipedHandle;

    constexpr std::string_view k_RecordingsVariable{ "ARTEMIS_RECORDINGS" };
    constexpr std::string_view k_SyntheticMap{ "synthetic.map" };

    // A recording in memory: the two files of one session.
    struct Recording
    {
        std::stringstream Sdump{};
        std::stringstream Tdump{};
    };

    // Writes the snapshots the way the DLL records them: the pipeline over each one, the .tdump from its Tick and the .sdump from
    // what is stored. The stored snapshots are the same ones unless a test wants a corrupt .sdump.
    // param flags: SnapshotRecordHeader::Flags of each record.
    auto Record(Chain& chain, const std::vector<Snapshot>& snapshots, const std::vector<Snapshot>& stored,
        const std::vector<std::uint32_t>& flags) -> Recording
    {
        Recording recording{};
        TickCodec::WriteFileHeader(recording.Tdump, "synthetic", "2026-10-27 12:00:00", "test");
        SnapshotCodec::WriteFileHeader(recording.Sdump, k_BridgeLayoutHash, "2026-10-27 12:00:00", "test");

        Bridge::Codec::Dump::Type::TickDumpWriterState tickState{};
        TickCodec::Begin(tickState);
        SnapshotWriterState snapshotState{};
        auto flat{ std::make_unique<BridgeTick>() };

        SnapshotLoadInfo info{ .MapFileSize = 1234 };
        std::ranges::copy(k_SyntheticMap, info.MapFileName.begin());

        for (std::size_t i{ 0 }; i < snapshots.size(); ++i)
        {
            const std::uint64_t generation{ i + 1 };
            chain.m_Export.m_PipelineService.Run(snapshots[i], generation, {});
            const auto tick{ chain.m_Export.m_TickStore.Acquire() };

            TickCodec::ClearHeader(*flat);
            chain.m_Export.m_FlatService.Flatten(*tick, *flat);
            TickCodec::WriteRecord(recording.Tdump, *flat, tick->Match != nullptr, 0, tickState);

            SnapshotCodec::WriteRecord(recording.Sdump, std::make_shared<const Snapshot>(stored[i]), generation, 0, flags[i], info,
                snapshotState);
        }

        TickCodec::WriteFooter(recording.Tdump, Bridge::Codec::Dump::Type::TickDumpCounters{ .RecordedTicks = snapshots.size() });
        SnapshotCodec::WriteFooter(recording.Sdump, SnapshotDumpFooter{ .Marker = k_SnapshotDumpEndMarker, .RecordedTicks = snapshots.size() });
        return recording;
    }

    // return: The biped of WithBipedAndPlayer moved to x = 11.
    auto WithBipedMoved() -> Snapshot
    {
        Snapshot snapshot{ WithBipedAndPlayer() };
        Put(snapshot.RawObjects.at(k_BipedHandle).Bytes, ObjeOffset::k_Origin, Vec3{ 11.0f, 20.0f, 30.0f });
        return snapshot;
    }

    // return: ARTEMIS_RECORDINGS if it is set, else Ignore/Regresion of the repository.
    auto RecordingsFolder() -> std::filesystem::path
    {
        char* value{ nullptr };
        std::size_t length{};
        std::filesystem::path folder{};

        if (_dupenv_s(&value, &length, k_RecordingsVariable.data()) == 0 && value != nullptr)
        {
            folder = value;
            std::free(value);
        }

        if (folder.empty())
        {
            // This file is Artemis/7_Tests/Tests/Perception/ReplayTests.cpp.
            const std::filesystem::path source{ std::source_location::current().file_name() };
            folder = source.parent_path().parent_path().parent_path().parent_path().parent_path() / "Ignore" / "Regresion";
        }

        return folder;
    }

    // return: The text of a fixed size array up to its first 0.
    template <std::size_t N>
    auto TextOf(const std::array<char, N>& text) -> std::string
    {
        return std::string{ text.begin(), std::ranges::find(text, '\0') };
    }

    // Loads the map of the recording as AIThread::LoadResources does.
    auto LoadMap(Chain& chain, const std::filesystem::path& mapPath) -> void
    {
        const std::filesystem::path folder{ mapPath.parent_path() };
        std::error_code error{};

        if (std::filesystem::is_regular_file(folder / "shared.map", error)) chain.m_Map.m_MapLoaderService.OpenFile((folder / "shared.map").string());
        if (std::filesystem::is_regular_file(folder / "campaign.map", error)) chain.m_Map.m_MapLoaderService.OpenFile((folder / "campaign.map").string());
        chain.m_Map.m_MapLoaderService.OpenFile(mapPath.string());

        chain.m_Map.m_MapBuilderService.LoadForMap();
        chain.m_Resolved.m_DefinitionsBuilder.BuildForMap();
        chain.m_Resolved.m_WorldBuilder.BuildForMap();
        chain.m_Resolved.m_VitalityBuilder.BuildForMap();
    }

    // What the Perception of the ctf_c recording gave at one engine tick.
    struct CtfMoment
    {
        std::int16_t RoundIndex{};
        std::array<Perception::Match::Type::MatchTeamScore, 8> TeamScores{};
        Perception::Details::Type::ObjectiveDetails Objectives{};
    };

    // What the Perception gave over the whole ctf_c recording that is not tied to one tick.
    struct CtfRun
    {
        // Events Artemis generated (EngineSequence 0), in order.
        std::vector<Perception::Events::Type::GameEvent> Generated{};
        // Events of the engine ring with the kind of a flag that reached the log: none should.
        std::size_t EngineFlagEvents{};
        // Whether the Sequence of the log grew by one with each event, from 1.
        bool IsSequenceContinuous{ true };
    };

    // Replays the recording of ctf_c record by record and keeps what the Perception gave at the engine ticks asked for.
    // param ticks: Engine ticks (Match.GameTick) to keep.
    // param last: Receives the moment of the last record.
    // param run: Receives the events of the whole recording.
    // return: Empty if it ran, else the reason it cannot run here (no recording, map not found).
    auto RunCtfRecording(const std::vector<std::uint32_t>& ticks, std::map<std::uint32_t, CtfMoment>& moments, CtfMoment& last, CtfRun& run) -> std::string
    {
        const std::filesystem::path directory{ RecordingsFolder() / "ctf_c" };
        std::error_code error{};
        if (!std::filesystem::is_regular_file(directory / "golden.sdump", error)) return "No ctf_c recording in " + directory.string();

        std::ifstream sdump{ directory / "golden.sdump", std::ios::binary };
        SnapshotReaderState reader{};
        if (const std::string reason{ SnapshotCodec::OpenReader(sdump, reader) }; !reason.empty()) return reason;
        auto next{ SnapshotCodec::ReadRecord(sdump, reader) };
        if (next.Status != SnapshotReadStatus::Record || !next.Record.LoadInfo) return "the .sdump does not start at a map load";

        const std::string mapName{ TextOf(next.Record.LoadInfo->MapFileName) };
        const std::filesystem::path mapPath{ MapsFolder() / mapName };
        if (!std::filesystem::is_regular_file(mapPath, error) || std::filesystem::file_size(mapPath, error) != next.Record.LoadInfo->MapFileSize)
        {
            return mapPath.string() + " not found or not the map of the recording";
        }

        auto chain{ std::make_unique<Chain>() };
        LoadMap(*chain, mapPath);

        std::uint32_t lastSequence{ 0 };
        for (bool isFirst{ true }; next.Status == SnapshotReadStatus::Record; isFirst = false)
        {
            if (!isFirst && (next.Record.Flags & k_SnapshotFirstAfterLoad) != 0) break;

            chain->m_Export.m_PipelineService.Run(*next.Record.Snapshot, next.Record.Generation, {});
            const auto match{ chain->m_Environment.m_MatchStore.Acquire() };
            const auto objectives{ chain->m_Environment.m_ObjectiveDetailsStore.Acquire() };
            if (match && objectives)
            {
                CtfMoment moment{ .RoundIndex = match->RoundIndex, .TeamScores = match->TeamScores, .Objectives = *objectives };
                if (std::ranges::contains(ticks, match->GameTick)) moments[match->GameTick] = moment;
                last = std::move(moment);
            }

            const auto log{ chain->m_Environment.m_EventsStore.Acquire() };
            if (log)
            {
                for (const auto& event : log->Rows)
                {
                    if (event.Sequence <= lastSequence) continue;

                    if (event.Sequence != lastSequence + 1) run.IsSequenceContinuous = false;
                    lastSequence = event.Sequence;

                    const bool isFlag{ event.Kind >= Common::Semantic::Type::Event::FlagTaken && event.Kind <= Common::Semantic::Type::Event::FlagCaptured };
                    if (event.EngineSequence == 0) run.Generated.push_back(event);
                    else if (isFlag) ++run.EngineFlagEvents;
                }
            }

            next = SnapshotCodec::ReadRecord(sdump, reader);
        }

        return {};
    }

    class ReplayTests : public ::testing::Test
    {
    protected:
        auto SetUp() -> void override
        {
            FillSyntheticTags(m_Recorder.m_Map);
            FillSyntheticTags(m_Replayer.m_Map);
        }

        // return: The three snapshots of the synthetic recording: nothing, a biped and a player, the biped moved.
        auto Snapshots() const -> std::vector<Snapshot>
        {
            return { Snapshot{}, WithBipedAndPlayer(), WithBipedMoved() };
        }

        Chain m_Recorder{};
        Chain m_Replayer{};
    };
}

TEST_F(ReplayTests, SyntheticRecordingReplaysEqual)
{
    const std::vector<Snapshot> snapshots{ this->Snapshots() };
    Recording recording{ Record(m_Recorder, snapshots, snapshots, { k_SnapshotFirstAfterLoad, 0, 0 }) };

    const ReplayResult result{ Replay(m_Replayer.m_Export, recording.Sdump, recording.Tdump) };

    EXPECT_EQ(result.Status, ReplayStatus::Equal) << result.Reason;
    EXPECT_EQ(result.Records, 3u);
}

TEST_F(ReplayTests, CorruptedObjectDiffers)
{
    const std::vector<Snapshot> snapshots{ this->Snapshots() };

    // The .sdump keeps the second snapshot with the sign of the x of the Origin of the biped flipped (one byte).
    std::vector<Snapshot> stored{ snapshots };
    stored[1].RawObjects.at(k_BipedHandle).Bytes[ObjeOffset::k_Origin + 3] ^= 0x80;

    Recording recording{ Record(m_Recorder, snapshots, stored, { k_SnapshotFirstAfterLoad, 0, 0 }) };

    const ReplayResult result{ Replay(m_Replayer.m_Export, recording.Sdump, recording.Tdump) };

    EXPECT_EQ(result.Status, ReplayStatus::Differs) << result.Reason;
    EXPECT_EQ(result.FirstDifference, 2u);
}

TEST_F(ReplayTests, NotFromLoadIsSkipped)
{
    const std::vector<Snapshot> snapshots{ this->Snapshots() };
    Recording recording{ Record(m_Recorder, snapshots, snapshots, { 0, 0, 0 }) };

    const ReplayResult result{ Replay(m_Replayer.m_Export, recording.Sdump, recording.Tdump) };

    EXPECT_EQ(result.Status, ReplayStatus::Skipped) << result.Reason;
    EXPECT_EQ(result.Records, 0u);
}

TEST_F(ReplayTests, SecondLoadStopsTheReplay)
{
    const std::vector<Snapshot> snapshots{ this->Snapshots() };
    Recording recording{ Record(m_Recorder, snapshots, snapshots, { k_SnapshotFirstAfterLoad, 0, k_SnapshotFirstAfterLoad }) };

    const ReplayResult result{ Replay(m_Replayer.m_Export, recording.Sdump, recording.Tdump) };

    EXPECT_EQ(result.Status, ReplayStatus::Equal) << result.Reason;
    EXPECT_EQ(result.Records, 2u);
}

// --- B4h H.2: the objectives of the ctf_c recording (3 rounds, a user gametype) ---

namespace
{
    using Common::Semantic::Type::FlagState;
    using Perception::Details::Type::ObjectiveDetail;

    // return: The flags of the moment that satisfy the condition.
    template <typename TCondition>
    auto FlagsWhere(const CtfMoment& moment, TCondition condition) -> std::vector<ObjectiveDetail>
    {
        std::vector<ObjectiveDetail> flags{};
        for (const auto& [handle, flag] : moment.Objectives.Flags)
        {
            if (condition(flag)) flags.push_back(flag);
        }

        return flags;
    }

    // return: The index of a player in the player table, from its handle.
    auto PlayerIndexOf(std::uint32_t handle) -> std::uint32_t
    {
        return handle & 0xFFFF;
    }
}

TEST_F(ReplayTests, CtfObjectivesFollowTheRecordedMatch)
{
    const std::vector<std::uint32_t> ticks{ 1950, 1960, 4283, 5000, 7870, 15000, 17700, 21300, 28600, 30000 };
    std::map<std::uint32_t, CtfMoment> moments{};
    CtfMoment last{};
    CtfRun run{};
    const std::string reason{ RunCtfRecording(ticks, moments, last, run) };
    if (!reason.empty()) GTEST_SKIP() << reason;

    for (const std::uint32_t tick : ticks) ASSERT_TRUE(moments.contains(tick)) << "no record at engine tick " << tick;

    // The flag of the blue base is carried by player 0 at 1950 and dropped at 1960.
    const auto blueAt1950{ FlagsWhere(moments.at(1950), [](const ObjectiveDetail& flag) { return flag.BaseTeam == Common::Team::Type::Team::Blue; }) };
    ASSERT_EQ(blueAt1950.size(), 1u);
    EXPECT_EQ(blueAt1950[0].State, FlagState::Carried);
    EXPECT_EQ(PlayerIndexOf(blueAt1950[0].CarrierPlayer), 0u);

    const auto blueAt1960{ FlagsWhere(moments.at(1960), [](const ObjectiveDetail& flag) { return flag.BaseTeam == Common::Team::Type::Team::Blue; }) };
    ASSERT_EQ(blueAt1960.size(), 1u);
    EXPECT_EQ(blueAt1960[0].State, FlagState::Dropped);

    // A dropped flag whose timer runs.
    EXPECT_FALSE(FlagsWhere(moments.at(4283), [](const ObjectiveDetail& flag) { return flag.State == FlagState::Dropped && flag.IsResetRunning; }).empty());

    // 12 ticks left of the reset timer (one tick of margin).
    EXPECT_FALSE(FlagsWhere(moments.at(7870), [](const ObjectiveDetail& flag) {
        return flag.State == FlagState::Dropped && flag.ResetTicks >= 11 && flag.ResetTicks <= 13; }).empty());

    // Round 2 return with one ally in the zone, and with two allies later.
    EXPECT_FALSE(FlagsWhere(moments.at(17700), [](const ObjectiveDetail& flag) { return flag.State == FlagState::Dropped && flag.AlliesInZone == 1; }).empty());
    EXPECT_FALSE(FlagsWhere(moments.at(21300), [](const ObjectiveDetail& flag) { return flag.State == FlagState::Dropped && flag.AlliesInZone == 2; }).empty());

    // A bot carries a flag (not player 0).
    EXPECT_FALSE(FlagsWhere(moments.at(28600), [](const ObjectiveDetail& flag) {
        return flag.State == FlagState::Carried && flag.CarrierPlayer != 0xFFFFFFFF && PlayerIndexOf(flag.CarrierPlayer) != 0; }).empty());

    EXPECT_EQ(moments.at(5000).RoundIndex, 0);
    EXPECT_EQ(moments.at(15000).RoundIndex, 1);
    EXPECT_EQ(moments.at(30000).RoundIndex, 2);

    // Three captures of one point: the totals of the match add up to 3.
    std::int32_t total{ 0 };
    for (const auto& score : last.TeamScores) total += score.Total;
    EXPECT_EQ(total, 3);
}

// --- B4h H.3: the flag events Artemis generates in the ctf_c recording ---

namespace
{
    using Common::Semantic::Type::Event;

    // A flag event of the expected list: the tick and the kind.
    struct ExpectedEvent
    {
        std::uint32_t Tick{};
        Event Kind{};
    };

    // return: The events of Ignore/Tareas/B4h_esperado_ctf_c.txt (lines "tick Kind"), empty if the file is not there.
    auto ReadExpectedEvents() -> std::vector<ExpectedEvent>
    {
        std::ifstream file{ RecordingsFolder().parent_path() / "Tareas" / "B4h_esperado_ctf_c.txt" };
        const std::map<std::string, Event> kinds{ { "Taken", Event::FlagTaken }, { "Dropped", Event::FlagDropped }, { "Returned", Event::FlagReturned },
            { "Reset", Event::FlagReset }, { "Captured", Event::FlagCaptured } };

        std::vector<ExpectedEvent> expected{};
        std::string line{};
        while (std::getline(file, line))
        {
            std::istringstream words{ line };
            std::uint32_t tick{};
            std::string kind{};
            if ((words >> tick >> kind) && kinds.contains(kind)) expected.push_back(ExpectedEvent{ tick, kinds.at(kind) });
        }

        return expected;
    }
}

TEST_F(ReplayTests, CtfFlagEventsMatchTheExpectedList)
{
    const std::vector<ExpectedEvent> expected{ ReadExpectedEvents() };
    if (expected.empty()) GTEST_SKIP() << "No Ignore/Tareas/B4h_esperado_ctf_c.txt";

    std::map<std::uint32_t, CtfMoment> moments{};
    CtfMoment last{};
    CtfRun run{};
    const std::string reason{ RunCtfRecording({}, moments, last, run) };
    if (!reason.empty()) GTEST_SKIP() << reason;

    EXPECT_EQ(run.EngineFlagEvents, 0u);
    EXPECT_TRUE(run.IsSequenceContinuous);

    // Taken and Dropped must be in the exact tick. Reset, Returned and Captured within 3 ticks. Compared kind by kind, in order.
    std::vector<std::string> differences{};
    for (const Event kind : { Event::FlagTaken, Event::FlagDropped, Event::FlagReturned, Event::FlagReset, Event::FlagCaptured })
    {
        std::vector<std::uint32_t> want{};
        for (const ExpectedEvent& event : expected) if (event.Kind == kind) want.push_back(event.Tick);

        std::vector<std::uint32_t> got{};
        for (const auto& event : run.Generated) if (event.Kind == kind) got.push_back(event.GameTick);

        const std::uint32_t tolerance{ kind == Event::FlagTaken || kind == Event::FlagDropped ? 0u : 3u };
        const std::string name{ std::to_string(static_cast<int>(kind)) };
        if (want.size() != got.size()) differences.push_back(std::format("kind {}: expected {} events, generated {}", name, want.size(), got.size()));

        for (std::size_t i{ 0 }; i < std::min(want.size(), got.size()); ++i)
        {
            const std::uint32_t distance{ want[i] > got[i] ? want[i] - got[i] : got[i] - want[i] };
            if (distance > tolerance) differences.push_back(std::format("kind {} number {}: expected tick {}, generated tick {}", name, i, want[i], got[i]));
        }
    }

    std::string text{};
    for (std::size_t i{ 0 }; i < std::min<std::size_t>(differences.size(), 20); ++i) text += differences[i] + "\n";
    EXPECT_TRUE(differences.empty()) << differences.size() << " differences, the first 20:\n" << text;
    EXPECT_EQ(run.Generated.size(), expected.size());
}

TEST_F(ReplayTests, Recordings)
{
    const std::filesystem::path folder{ RecordingsFolder() };

    std::vector<std::filesystem::path> recordings{};
    std::error_code error{};
    for (const auto& entry : std::filesystem::directory_iterator(folder, error))
    {
        if (entry.is_directory() && std::filesystem::is_regular_file(entry.path() / "golden.sdump", error) &&
            std::filesystem::is_regular_file(entry.path() / "golden.tdump", error))
        {
            recordings.push_back(entry.path());
        }
    }

    if (recordings.empty()) GTEST_SKIP() << "No recording (golden.sdump and golden.tdump) in " << folder.string();

    std::size_t replayed{ 0 };
    std::string skipped{};

    for (const std::filesystem::path& directory : recordings)
    {
        SCOPED_TRACE(directory.string());

        std::ifstream sdump{ directory / "golden.sdump", std::ios::binary };
        std::ifstream tdump{ directory / "golden.tdump", std::ios::binary };

        // The map the recording was made with.
        SnapshotReaderState reader{};
        const std::string openReason{ SnapshotCodec::OpenReader(sdump, reader) };
        const auto first{ openReason.empty() ? SnapshotCodec::ReadRecord(sdump, reader) : Blam::Layout::Snapshot::Type::SnapshotReadResult{} };
        if (!openReason.empty() || first.Status != SnapshotReadStatus::Record || !first.Record.LoadInfo)
        {
            ADD_FAILURE() << directory.string() << ": the .sdump cannot be read or does not start at a map load. " << openReason << first.Reason;
            continue;
        }

        const std::string mapName{ TextOf(first.Record.LoadInfo->MapFileName) };
        const std::filesystem::path mapPath{ MapsFolder() / mapName };
        const std::uintmax_t mapSize{ std::filesystem::is_regular_file(mapPath, error) ? std::filesystem::file_size(mapPath, error) : 0 };
        if (mapSize == 0)
        {
            skipped += mapPath.string() + " not found. ";
            continue;
        }
        if (mapSize != first.Record.LoadInfo->MapFileSize)
        {
            skipped += std::format("{} has {} bytes, the recording was made with {}. ", mapPath.string(), mapSize, first.Record.LoadInfo->MapFileSize);
            continue;
        }

        // The footer holds the CRC-32 of the map of the session. A file without footer or without CRC is checked by size only.
        SnapshotDumpFooter footer{};
        sdump.clear();
        sdump.seekg(-static_cast<std::streamoff>(sizeof(footer)), std::ios::end);
        sdump.read(reinterpret_cast<char*>(&footer), sizeof(footer));
        if (sdump.gcount() == static_cast<std::streamsize>(sizeof(footer)) && footer.Marker == k_SnapshotDumpEndMarker &&
            footer.HasCrc != 0 && TextOf(footer.MapFileName) == mapName)
        {
            const std::optional<std::uint32_t> crc{ FileCrc32(mapPath) };
            if (!crc || *crc != footer.MapCrc32)
            {
                skipped += std::format("{} has another CRC-32 than the recording ({:08X}). ", mapPath.string(), footer.MapCrc32);
                continue;
            }
        }
        else
        {
            std::cout << "[  WARNING ] " << directory.string() << ": no CRC-32 of the map in the footer, checked by size only.\n";
        }

        auto chain{ std::make_unique<Chain>() };
        LoadMap(*chain, mapPath);

        sdump.clear();
        sdump.seekg(0);
        const ReplayResult result{ Replay(chain->m_Export, sdump, tdump) };

        if (result.Status == ReplayStatus::Skipped)
        {
            skipped += directory.string() + ": " + result.Reason + ". ";
            continue;
        }

        ++replayed;
        if (result.Status == ReplayStatus::Differs)
        {
            // The header of the recording and the records the pipeline gave, to compare them with tools/tickdump.py diff.
            std::ifstream golden{ directory / "golden.tdump", std::ios::binary };
            TickDumpFileHeader header{};
            golden.read(reinterpret_cast<char*>(&header), sizeof(header));
            std::vector<char> prefix(header.HeaderSize + header.LayoutJsonSize);
            golden.seekg(0);
            golden.read(prefix.data(), static_cast<std::streamsize>(prefix.size()));

            std::ofstream replay{ directory / "replay.tdump", std::ios::binary | std::ios::trunc };
            replay.write(prefix.data(), static_cast<std::streamsize>(prefix.size()));
            replay.write(result.Replayed.data(), static_cast<std::streamsize>(result.Replayed.size()));

            ADD_FAILURE() << std::format("record {} (GameTick {}): python Tools/tickdump.py diff golden.tdump replay.tdump",
                result.FirstDifference, result.GameTick);
        }
        else if (result.Status != ReplayStatus::Equal)
        {
            ADD_FAILURE() << result.Reason;
        }
    }

    if (replayed == 0 && !::testing::Test::HasFailure()) GTEST_SKIP() << skipped;
}