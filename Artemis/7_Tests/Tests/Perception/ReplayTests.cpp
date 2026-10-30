#include <gtest/gtest.h>

#include <stdlib.h>

import Common.Engine.Type;
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
        std::uint32_t RoundClockTicks{};
        std::int32_t RoundTicksRemaining{};
        std::uint32_t RoundTimeLimitTicks{};
        std::uint8_t RoundLimit{};
        std::int32_t ScoreToWin{};
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

    // Replays the recording of a capture film record by record and keeps what the Perception gave at the engine ticks asked for.
    // param film: Folder of Ignore/Regresion (ctf_c, cgb_displace_1min).
    // param ticks: Engine ticks (Match.GameTick) to keep.
    // param last: Receives the moment of the last record.
    // param run: Receives the events of the whole recording.
    // return: Empty if it ran, else the reason it cannot run here (no recording, map not found).
    auto RunCtfRecording(std::string_view film, const std::vector<std::uint32_t>& ticks, std::map<std::uint32_t, CtfMoment>& moments, CtfMoment& last, CtfRun& run) -> std::string
    {
        const std::filesystem::path directory{ RecordingsFolder() / film };
        std::error_code error{};
        if (!std::filesystem::is_regular_file(directory / "golden.sdump", error)) return "No recording in " + directory.string();

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
                CtfMoment moment{ .RoundIndex = match->RoundIndex, .TeamScores = match->TeamScores, .Objectives = *objectives,
                    .RoundClockTicks = match->RoundClockTicks, .RoundTicksRemaining = match->RoundTicksRemaining,
                    .RoundTimeLimitTicks = match->RoundTimeLimitTicks, .RoundLimit = match->RoundLimit, .ScoreToWin = match->ScoreToWin };
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
    const std::string reason{ RunCtfRecording("ctf_c", ticks, moments, last, run) };
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
    const std::string reason{ RunCtfRecording("ctf_c", {}, moments, last, run) };
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

// --- B4h H.4: the flags of the cgb_displace_1min film. Two players run with the enemy flags: the red flag is taken, dropped and taken
// again six times (the user watched this one), the blue flag too. The engine recreates the flag every time it is dropped, with no
// Megalo element for about ten ticks, and neither that nor the lying on the ground is a reset or a return. After the last drop both flags
// lie on the ground until the end of the film. ---

TEST_F(ReplayTests, CgbFlagsAreTakenAndDroppedSixTimesEachAndNothingElse)
{
    std::map<std::uint32_t, CtfMoment> moments{};
    CtfMoment last{};
    CtfRun run{};
    const std::string reason{ RunCtfRecording("cgb_displace_1min", {}, moments, last, run) };
    if (!reason.empty()) GTEST_SKIP() << reason;

    EXPECT_EQ(run.EngineFlagEvents, 0u);
    EXPECT_TRUE(run.IsSequenceContinuous);

    // Ticks of each kind by the team that plays the side of the flag (the mapping of the film is red 0, blue 1).
    std::map<std::pair<std::uint8_t, Event>, std::vector<std::uint32_t>> ticks{};
    for (const auto& event : run.Generated) ticks[{ event.Modifier, event.Kind }].push_back(event.GameTick);

    using Ticks = std::vector<std::uint32_t>;
    EXPECT_EQ((ticks[{ 0, Event::FlagTaken }]), (Ticks{ 1970, 2082, 2195, 2311, 2434, 2611 }));
    EXPECT_EQ((ticks[{ 0, Event::FlagDropped }]), (Ticks{ 1996, 2136, 2218, 2339, 2460, 2630 }));
    EXPECT_EQ((ticks[{ 1, Event::FlagTaken }]), (Ticks{ 1911, 2023, 2148, 2274, 2329, 2440 }));
    EXPECT_EQ((ticks[{ 1, Event::FlagDropped }]), (Ticks{ 1959, 2082, 2196, 2298, 2395, 2492 }));
    EXPECT_EQ(run.Generated.size(), 24u);

    // They end lying on the ground: no reset, no return and no capture came.
    ASSERT_EQ(last.Objectives.Flags.size(), 2u);
    for (const auto& [handle, flag] : last.Objectives.Flags) EXPECT_EQ(flag.State, FlagState::Dropped) << handle;
}

// --- R5 Q89: the round time and the round limit of the game variant ---

TEST_F(ReplayTests, CtfCRoundClockIsAStopwatchWithoutLimit)
{
    const std::vector<std::uint32_t> ticks{ 3001, 11502, 11563, 15001 };
    std::map<std::uint32_t, CtfMoment> moments{};
    CtfMoment last{};
    CtfRun run{};
    const std::string reason{ RunCtfRecording("ctf_c", ticks, moments, last, run) };
    if (!reason.empty()) GTEST_SKIP() << reason;

    for (const std::uint32_t tick : ticks) ASSERT_TRUE(moments.contains(tick)) << "no record at engine tick " << tick;

    EXPECT_EQ(moments.at(3001).RoundClockTicks, 2280u);
    // Frozen when the round ends and back to 0 when the map is recreated.
    EXPECT_EQ(moments.at(11502).RoundClockTicks, 10071u);
    EXPECT_EQ(moments.at(11563).RoundClockTicks, 0u);
    EXPECT_EQ(moments.at(15001).RoundClockTicks, 2695u);

    for (const std::uint32_t tick : ticks)
    {
        EXPECT_EQ(moments.at(tick).RoundLimit, 3) << tick;
        EXPECT_EQ(moments.at(tick).RoundTimeLimitTicks, 0u) << tick;
        EXPECT_EQ(moments.at(tick).RoundTicksRemaining, -1) << tick;
        EXPECT_EQ(moments.at(tick).ScoreToWin, 1) << tick;
    }
}

TEST_F(ReplayTests, CgbRoundHasOneRoundAndItsClockWaitsForTheEndOfThePreRound)
{
    const std::vector<std::uint32_t> ticks{ 100, 604 };
    std::map<std::uint32_t, CtfMoment> moments{};
    CtfMoment last{};
    CtfRun run{};
    const std::string reason{ RunCtfRecording("cgb_displace_1min", ticks, moments, last, run) };
    if (!reason.empty()) GTEST_SKIP() << reason;

    for (const std::uint32_t tick : ticks)
    {
        ASSERT_TRUE(moments.contains(tick)) << "no record at engine tick " << tick;
        EXPECT_EQ(moments.at(tick).RoundLimit, 1) << tick;
        EXPECT_EQ(moments.at(tick).ScoreToWin, 5) << tick;
        EXPECT_EQ(moments.at(tick).RoundClockTicks, 0u) << tick;
    }
}

TEST_F(ReplayTests, CtfDRoundCountsDownFromTheTimeLimit)
{
    const std::vector<std::uint32_t> ticks{ 1501, 7801, 8161, 9001, 12656 };
    std::map<std::uint32_t, CtfMoment> moments{};
    CtfMoment last{};
    CtfRun run{};
    const std::string reason{ RunCtfRecording("ctf_d", ticks, moments, last, run) };
    if (!reason.empty()) GTEST_SKIP() << reason;

    for (const std::uint32_t tick : ticks)
    {
        ASSERT_TRUE(moments.contains(tick)) << "no record at engine tick " << tick;
        EXPECT_EQ(moments.at(tick).RoundTimeLimitTicks, 7200u) << tick;
        EXPECT_EQ(moments.at(tick).RoundLimit, 3) << tick;
        EXPECT_EQ(moments.at(tick).ScoreToWin, 1) << tick;
    }

    EXPECT_EQ(moments.at(1501).RoundTicksRemaining, 6300);
    EXPECT_EQ(moments.at(1501).RoundClockTicks, 900u);
    // Round 1 ran out of time.
    EXPECT_EQ(moments.at(7801).RoundTicksRemaining, 0);
    EXPECT_EQ(moments.at(7801).RoundClockTicks, 7200u);
    // The map was recreated: the clock is loaded with the whole limit.
    EXPECT_EQ(moments.at(8161).RoundTicksRemaining, 7200);
    EXPECT_EQ(moments.at(9001).RoundTicksRemaining, 6961);
    // Round 2 ended by a capture: frozen.
    EXPECT_EQ(moments.at(12656).RoundTicksRemaining, 3606);
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
        const std::filesystem::path replayPath{ directory / "replay.tdump" };
        if (result.Status == ReplayStatus::Differs || result.Status == ReplayStatus::LayoutChanged)
        {
            // The records the pipeline gave, in a complete .tdump to compare with tools/tickdump.py diff and to approve with
            // tools/tickdump.py approve: the name, the start and the source of the recording, the layout of this build and a footer with
            // the counters of the run (a replay drops no tick and loses no event).
            std::ifstream golden{ directory / "golden.tdump", std::ios::binary };
            TickDumpFileHeader header{};
            golden.read(reinterpret_cast<char*>(&header), sizeof(header));

            std::ofstream replay{ replayPath, std::ios::binary | std::ios::trunc };
            TickCodec::WriteFileHeader(replay, TextOf(header.MapName), TextOf(header.StartTime), TextOf(header.Source));
            replay.write(result.Replayed.data(), static_cast<std::streamsize>(result.Replayed.size()));
            TickCodec::WriteFooter(replay, Bridge::Codec::Dump::Type::TickDumpCounters{ .RecordedTicks = result.Records });

            if (result.Status == ReplayStatus::LayoutChanged)
            {
                ADD_FAILURE() << result.Reason << ": python Tools/tickdump.py diff golden.tdump replay.tdump";
            }
            else
            {
                ADD_FAILURE() << std::format("record {} (GameTick {}): python Tools/tickdump.py diff golden.tdump replay.tdump",
                    result.FirstDifference, result.GameTick);
            }
        }
        else if (result.Status != ReplayStatus::Equal)
        {
            ADD_FAILURE() << result.Reason;
        }
        else
        {
            // A replay.tdump left by an earlier difference would be taken for the result of this run.
            std::filesystem::remove(replayPath, error);
        }
    }

    if (replayed == 0 && !::testing::Test::HasFailure()) GTEST_SKIP() << skipped;
}

// --- W.3: the weapons and projectiles of the armas_a recording ---

namespace
{
    // What the Perception gave about the weapons and the projectiles at one engine tick of armas_a.
    struct ArmasMoment
    {
        Perception::Details::Type::WeaponDetails Weapons{};
        Perception::Details::Type::ProjectileDetails Projectiles{};
    };

    // Replays the armas_a recording record by record and keeps the weapon and projectile details at the engine ticks asked for.
    // return: Empty if it ran, else the reason it cannot run here (no recording, map not found).
    auto RunArmasRecording(const std::vector<std::uint32_t>& ticks, std::map<std::uint32_t, ArmasMoment>& moments) -> std::string
    {
        const std::filesystem::path directory{ RecordingsFolder() / "armas_a" };
        std::error_code error{};
        if (!std::filesystem::is_regular_file(directory / "golden.sdump", error)) return "No recording in " + directory.string();

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

        for (bool isFirst{ true }; next.Status == SnapshotReadStatus::Record; isFirst = false)
        {
            if (!isFirst && (next.Record.Flags & k_SnapshotFirstAfterLoad) != 0) break;

            chain->m_Export.m_PipelineService.Run(*next.Record.Snapshot, next.Record.Generation, {});
            const auto match{ chain->m_Environment.m_MatchStore.Acquire() };
            const auto weapons{ chain->m_Environment.m_WeaponDetailsStore.Acquire() };
            const auto projectiles{ chain->m_Environment.m_ProjectileDetailsStore.Acquire() };
            if (match && weapons && projectiles && std::ranges::contains(ticks, match->GameTick))
            {
                moments[match->GameTick] = ArmasMoment{ .Weapons = *weapons, .Projectiles = *projectiles };
            }

            next = SnapshotCodec::ReadRecord(sdump, reader);
        }

        return {};
    }

    constexpr std::uint32_t k_Player0{ 0xEC700000 };
    constexpr std::uint32_t k_Biped0{ 0xE43301C4 };
    constexpr std::uint32_t k_Dmr{ 0xE43701C8 };
    constexpr std::uint32_t k_PlasmaRifle{ 0xE42201B3 };
    constexpr std::uint32_t k_PlasmaPistol{ 0xE42301B4 };
    constexpr std::uint32_t k_Rocket{ 0xE42401B5 };
    constexpr std::uint32_t k_RocketTarget{ 0xE42801B9 };
    constexpr std::uint32_t k_PlasmaGrenade{ 0xE4E301CF };
    constexpr std::uint32_t k_GrenadeVictim{ 0xE4D501BA };
    constexpr std::uint32_t k_Needle{ 0xE4FD01B6 };
    constexpr std::uint32_t k_NeedleVictim{ 0xE4F001CF };
    constexpr std::uint32_t k_RocketProjectile{ 0xE4C501C7 };
}

TEST_F(ReplayTests, ArmasAWeaponsAndProjectilesFollowTheRecordedFilm)
{
    using Common::Semantic::Type::ChargeState;
    using Common::Engine::Type::k_NoHandle;

    const std::vector<std::uint32_t> ticks{ 2250, 2300, 2580, 2640, 3300, 4420, 4500, 4540, 5000, 5880, 5950, 6231, 6300, 6410,
        7300, 7400, 7480, 8000, 8500, 8550, 8560, 10765, 10780, 10800, 13790, 13810 };
    std::map<std::uint32_t, ArmasMoment> moments{};
    const std::string reason{ RunArmasRecording(ticks, moments) };
    if (!reason.empty()) GTEST_SKIP() << reason;

    for (const std::uint32_t tick : ticks) ASSERT_TRUE(moments.contains(tick)) << "no record at engine tick " << tick;

    const auto weapon{ [&](std::uint32_t tick, std::uint32_t handle) -> const Perception::Details::Type::WeaponDetail&
    {
        const auto& weapons{ moments.at(tick).Weapons };
        EXPECT_TRUE(weapons.contains(handle)) << std::format("no weapon {:08X} at {}", handle, tick);
        static const Perception::Details::Type::WeaponDetail missing{};
        return weapons.contains(handle) ? weapons.at(handle) : missing;
    } };
    const auto projectile{ [&](std::uint32_t tick, std::uint32_t handle) -> const Perception::Details::Type::ProjectileDetail&
    {
        const auto& projectiles{ moments.at(tick).Projectiles };
        EXPECT_TRUE(projectiles.contains(handle)) << std::format("no projectile {:08X} at {}", handle, tick);
        static const Perception::Details::Type::ProjectileDetail missing{};
        return projectiles.contains(handle) ? projectiles.at(handle) : missing;
    } };

    // DMR: drawn, ready, reloading (the reload state outlasts the reload), ready again, empty and reloading.
    EXPECT_EQ(weapon(2250, k_Dmr).MagazineCount, 1);
    EXPECT_FALSE(weapon(2250, k_Dmr).IsEnergyWeapon);
    EXPECT_FALSE(weapon(2250, k_Dmr).CanFireNow);
    EXPECT_FALSE(weapon(2250, k_Dmr).IsReloading);
    EXPECT_EQ(weapon(2300, k_Dmr).Magazines[0].Loaded, 15);
    EXPECT_TRUE(weapon(2300, k_Dmr).CanFireNow);
    EXPECT_TRUE(weapon(2580, k_Dmr).IsReloading);
    EXPECT_FALSE(weapon(2580, k_Dmr).CanFireNow);
    EXPECT_EQ(weapon(2580, k_Dmr).Magazines[0].Loaded, 10);
    EXPECT_FALSE(weapon(2640, k_Dmr).IsReloading);
    EXPECT_TRUE(weapon(2640, k_Dmr).CanFireNow);
    EXPECT_EQ(weapon(2640, k_Dmr).Magazines[0].Loaded, 15);
    EXPECT_TRUE(weapon(3300, k_Dmr).IsReloading);
    EXPECT_EQ(weapon(3300, k_Dmr).Magazines[0].Loaded, 0);

    // Plasma rifle: overheated in its two phases, ready when the second ends.
    EXPECT_EQ(weapon(4420, k_PlasmaRifle).MagazineCount, 0);
    EXPECT_TRUE(weapon(4420, k_PlasmaRifle).IsEnergyWeapon);
    EXPECT_FLOAT_EQ(weapon(4420, k_PlasmaRifle).Energy, 1.0f);
    EXPECT_TRUE(weapon(4420, k_PlasmaRifle).IsOverheated);
    EXPECT_FALSE(weapon(4420, k_PlasmaRifle).CanFireNow);
    EXPECT_TRUE(weapon(4500, k_PlasmaRifle).IsOverheated);
    EXPECT_LT(weapon(4500, k_PlasmaRifle).Heat, 0.4f);
    EXPECT_FALSE(weapon(4540, k_PlasmaRifle).IsOverheated);
    EXPECT_TRUE(weapon(4540, k_PlasmaRifle).CanFireNow);
    EXPECT_TRUE(weapon(5000, k_PlasmaRifle).IsOverheated);

    // Plasma pistol: charge, released early, charge to the end, overheated after the charged shot.
    EXPECT_EQ(weapon(5880, k_PlasmaPistol).ChargeState, ChargeState::Charging);
    EXPECT_NEAR(weapon(5880, k_PlasmaPistol).ChargeFraction, 6.0f / 48.0f, 1e-5f);
    EXPECT_EQ(weapon(5950, k_PlasmaPistol).ChargeState, ChargeState::None);
    EXPECT_FLOAT_EQ(weapon(5950, k_PlasmaPistol).ChargeFraction, 0.0f);
    EXPECT_EQ(weapon(6231, k_PlasmaPistol).ChargeState, ChargeState::Charging);
    EXPECT_NEAR(weapon(6231, k_PlasmaPistol).ChargeFraction, 24.0f / 48.0f, 1e-5f);
    EXPECT_EQ(weapon(6300, k_PlasmaPistol).ChargeState, ChargeState::Charged);
    EXPECT_FLOAT_EQ(weapon(6300, k_PlasmaPistol).ChargeFraction, 1.0f);
    EXPECT_TRUE(weapon(6410, k_PlasmaPistol).IsOverheated);
    EXPECT_FALSE(weapon(6410, k_PlasmaPistol).CanFireNow);

    // Rocket launcher: aims at a Falcon, locks, loses the lock (state 3) keeping the target, then no target, locks again.
    EXPECT_EQ(weapon(7300, k_Rocket).TrackedObject, k_RocketTarget);
    EXPECT_FALSE(weapon(7300, k_Rocket).IsLocked);
    EXPECT_TRUE(weapon(7400, k_Rocket).IsLocked);
    EXPECT_FALSE(weapon(7480, k_Rocket).IsLocked);
    EXPECT_EQ(weapon(7480, k_Rocket).TrackedObject, k_RocketTarget);
    EXPECT_EQ(weapon(8000, k_Rocket).TrackedObject, k_NoHandle);
    EXPECT_TRUE(weapon(8500, k_Rocket).IsLocked);
    EXPECT_EQ(weapon(8560, k_Rocket).Magazines[0].Loaded, 1);

    // Projectiles: a plasma grenade in the hand of its thrower, thrown, stuck to a victim.
    EXPECT_TRUE(projectile(10765, k_PlasmaGrenade).InHand);
    EXPECT_EQ(projectile(10765, k_PlasmaGrenade).StuckTo, k_NoHandle);
    EXPECT_EQ(projectile(10765, k_PlasmaGrenade).OwnerBiped, k_Biped0);
    EXPECT_EQ(projectile(10765, k_PlasmaGrenade).OwnerPlayer, k_Player0);
    EXPECT_FALSE(projectile(10780, k_PlasmaGrenade).InHand);
    EXPECT_EQ(projectile(10780, k_PlasmaGrenade).StuckTo, k_NoHandle);
    EXPECT_EQ(projectile(10800, k_PlasmaGrenade).StuckTo, k_GrenadeVictim);
    EXPECT_GE(projectile(10800, k_PlasmaGrenade).StuckNode, 0);
    EXPECT_EQ(projectile(13790, k_Needle).StuckTo, k_NoHandle);
    EXPECT_EQ(projectile(13810, k_Needle).StuckTo, k_NeedleVictim);
    EXPECT_EQ(projectile(13810, k_Needle).OwnerPlayer, k_Player0);
    EXPECT_EQ(projectile(8550, k_RocketProjectile).OwnerBiped, k_Biped0);
    EXPECT_EQ(projectile(8550, k_RocketProjectile).ShooterRoot, k_Biped0);
    // Entity.Team of the rocket at 8550 is the team of player 0: it waits for W.4.
}