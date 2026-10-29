#include <gtest/gtest.h>

#include <stdlib.h>

import Common.Hash.System;
import Common.Math.Type;
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