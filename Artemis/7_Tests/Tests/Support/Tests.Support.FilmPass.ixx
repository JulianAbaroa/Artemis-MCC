module;

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <psapi.h>
#include <stdlib.h>

#pragma comment(lib, "psapi.lib")

export module Tests.Support.FilmPass;

import Common.Hash.System;
import Blam.Layout.Object.Type;
import Blam.Layout.Snapshot.Type;
import Blam.Layout.Snapshot.System;
import Bridge.Codec.Dump.Type;
import Bridge.Codec.Dump.System;
import Tests.Support.Maps;
import Tests.Support.Pipeline;
import Tests.Support.Replay;
import std;

// One pass over a golden .sdump (D269): the film goes through the pipeline once, and after each record the observers of the caller
// and the comparison with golden.tdump (if asked) see the result. The tests gather what they need in the observers and assert on it
// afterwards, so a film is never replayed twice by one process.

namespace
{
    namespace SnapshotCodec = ::Blam::Layout::Snapshot::System;
    namespace TickCodec = Bridge::Codec::Dump::System;

    using Bridge::Codec::Dump::Type::TickDumpCounters;
    using Bridge::Codec::Dump::Type::TickDumpFileHeader;
    using ::Blam::Layout::Snapshot::Type::RegionId;
    using ::Blam::Layout::Snapshot::Type::Snapshot;
    using ::Blam::Layout::Snapshot::Type::SnapshotDumpFooter;
    using ::Blam::Layout::Snapshot::Type::SnapshotLoadInfo;
    using ::Blam::Layout::Snapshot::Type::SnapshotReaderState;
    using ::Blam::Layout::Snapshot::Type::SnapshotReadResult;
    using ::Blam::Layout::Snapshot::Type::SnapshotReadStatus;
    using Common::Hash::System::FileCrc32;
    using Tests::Support::Maps::MapsFolder;
    using Tests::Support::Pipeline::Chain;
    using Tests::Support::Replay::ReplayComparer;
    using Tests::Support::Replay::ReplayResult;
    using Tests::Support::Replay::ReplayStatus;

    using ::Blam::Layout::Snapshot::Type::k_SnapshotDumpEndMarker;
    using ::Blam::Layout::Snapshot::Type::k_SnapshotFirstAfterLoad;
    using ::Blam::Layout::Object::Type::Constant::k_TimeGlobalsEngineTickOffset;

    constexpr std::string_view k_RecordingsVariable{ "ARTEMIS_RECORDINGS" };

    // return: The engine tick of a record, read from its time globals. 0 when the record has none.
    auto EngineTickOf(const Snapshot& snapshot) -> std::uint32_t
    {
        const std::vector<std::uint8_t>& bytes{ snapshot.Regions[static_cast<std::size_t>(RegionId::TimeGlobals)].Bytes };
        if (bytes.size() < k_TimeGlobalsEngineTickOffset + sizeof(std::uint32_t)) return 0;

        std::uint32_t tick{};
        std::memcpy(&tick, bytes.data() + k_TimeGlobalsEngineTickOffset, sizeof(tick));
        return tick;
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

}

export namespace Tests::Support::FilmPass
{
    // What a pass gave besides what the observers gathered.
    struct FilmOutcome
    {
        // Why the pass could not run, in the words of the tests of one film. Empty when it ran.
        std::string Skip{};
        // The same cause in the words of Recordings: what its skipped list gets (also set when only the comparison is dropped).
        std::string RecordingsSkip{};
        // The .sdump cannot be read: what Recordings reports as a failure.
        std::string RecordingsFailure{};
        // A note for the log of Recordings.
        std::string Warning{};
        // Why the pass stopped in the middle (an exception). The observers hold what they saw until then.
        std::string Error{};
        // Records replayed.
        std::size_t Records{};
        double Seconds{};
        // Whether the records were compared with golden.tdump. Comparison is set then.
        bool IsCompared{};
        // The records the pipeline gave are written to replay.tdump when they differ, and released.
        ReplayResult Comparison{};
    };

    // Called after the pipeline ran each record, on the thread of the pass. It only gathers data: no EXPECT, ASSERT or output.
    // param record: Number of the record, counted from 1.
    using FilmObserver = std::function<void(const Chain& chain, std::size_t record)>;

    // Called before the pipeline runs each record, on the thread of the pass, with the Snapshot the pipeline is about to get. The chain
    // holds what the previous record left. It may measure on its own copies, never change the chain.
    // return: The endStage of PipelineService::Run for this record (called after each stage), or empty for none.
    using FilmBeforeRecord = std::function<std::function<void()>(const Chain& chain, const ::Blam::Layout::Snapshot::Type::Snapshot& snapshot, std::size_t record)>;

    // A second pass beside the full one that works as the DLL does in sparse mode (Q101): a second chain whose pipeline only runs on the
    // decision records and the first record after the load, the other records go through MakeLight and PipelineService::RunLight.
    struct FilmSparse
    {
        // A record is a decision when its engine tick is a multiple of this and differs from the previous record's. 0 = no sparse pass.
        std::size_t Period{ 0 };
        // Called on each decision record after both chains ran it, on the thread of the pass. It only gathers data.
        std::function<void(Chain& full, Chain& sparse, std::size_t record)> OnDecision{};
    };
}

namespace
{
    using Tests::Support::FilmPass::FilmOutcome;

    // Checks that the map the recording was made with is on this PC. With isCompared, the CRC-32 of the footer is checked too.
    // param isCompared: Set to false when the CRC-32 differs, the comparison with golden.tdump is not made then.
    // return: false if the pass cannot run (the reasons are in the outcome).
    auto CheckMap(const std::filesystem::path& directory, const SnapshotLoadInfo& info, bool& isCompared, FilmOutcome& outcome) -> bool
    {
        const std::string mapName{ TextOf(info.MapFileName) };
        const std::filesystem::path mapPath{ MapsFolder() / mapName };
        std::error_code error{};
        const std::uintmax_t mapSize{ std::filesystem::is_regular_file(mapPath, error) ? std::filesystem::file_size(mapPath, error) : 0 };
        if (mapSize == 0 || mapSize != info.MapFileSize)
        {
            outcome.Skip = mapPath.string() + " not found or not the map of the recording";
            outcome.RecordingsSkip = mapSize == 0 ? mapPath.string() + " not found. " :
                std::format("{} has {} bytes, the recording was made with {}. ", mapPath.string(), mapSize, info.MapFileSize);
            return false;
        }
        if (!isCompared) return true;

        // The footer holds the CRC-32 of the map of the session. A file without footer or without CRC is checked by size only.
        std::ifstream tail{ directory / "golden.sdump", std::ios::binary };
        SnapshotDumpFooter footer{};
        tail.seekg(-static_cast<std::streamoff>(sizeof(footer)), std::ios::end);
        tail.read(reinterpret_cast<char*>(&footer), sizeof(footer));
        if (tail.gcount() == static_cast<std::streamsize>(sizeof(footer)) && footer.Marker == k_SnapshotDumpEndMarker &&
            footer.HasCrc != 0 && TextOf(footer.MapFileName) == mapName)
        {
            const std::optional<std::uint32_t> crc{ FileCrc32(mapPath) };
            if (!crc || *crc != footer.MapCrc32)
            {
                outcome.RecordingsSkip = std::format("{} has another CRC-32 than the recording ({:08X}). ", mapPath.string(), footer.MapCrc32);
                isCompared = false;
            }
        }
        else
        {
            outcome.Warning = directory.string() + ": no CRC-32 of the map in the footer, checked by size only.";
        }

        return true;
    }

    // Writes the records the pipeline gave in a complete .tdump to compare with tools/tickdump.py diff and to approve with
    // tools/tickdump.py approve: the name, the start and the source of the recording, the layout of this build and a footer with
    // the counters of the run (a replay drops no tick and loses no event).
    auto WriteReplay(const std::filesystem::path& directory, const ReplayResult& result) -> void
    {
        std::ifstream golden{ directory / "golden.tdump", std::ios::binary };
        TickDumpFileHeader header{};
        golden.read(reinterpret_cast<char*>(&header), sizeof(header));

        std::ofstream replay{ directory / "replay.tdump", std::ios::binary | std::ios::trunc };
        TickCodec::WriteFileHeader(replay, TextOf(header.MapName), TextOf(header.StartTime), TextOf(header.Source));
        replay.write(result.Replayed.data(), static_cast<std::streamsize>(result.Replayed.size()));
        TickCodec::WriteFooter(replay, TickDumpCounters{ .RecordedTicks = result.Records });
    }
}

export namespace Tests::Support::FilmPass
{
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
            // This file is Artemis/7_Tests/Tests/Support/Tests.Support.FilmPass.ixx.
            const std::filesystem::path source{ std::source_location::current().file_name() };
            folder = source.parent_path().parent_path().parent_path().parent_path().parent_path() / "Ignore" / "Regresion";
        }

        return folder;
    }

    // Replays the recording of a film once, from its start to the next map load, and calls the observer after each record.
    // param film: Folder of Ignore/Regresion.
    // param isCompared: Compare each record with golden.tdump in the same pass (D204). replay.tdump is written when they differ and
    // deleted when they are equal.
    // param before: Called before each record is run. Empty to skip it.
    // param sampleEvery: Only the records 1, 1 + sampleEvery, 1 + 2 * sampleEvery... go through the pipeline, the observer and the comparison.
    // The others are read and dropped, as the DLL does with the ticks it does not process in sparse mode (Q101). 1 runs them all.
    // param sparse: With a Period, every record also goes through the sparse chain (see FilmSparse) and OnDecision sees both chains. Use it with
    // sampleEvery 1.
    // note: Safe to call from several threads at once, each pass has its own Chain. An exception ends up in the outcome.
    auto RunFilmPass(std::string_view film, bool isCompared, const FilmObserver& observer, const FilmBeforeRecord& before = {}, std::size_t sampleEvery = 1,
        const FilmSparse& sparse = {}) -> FilmOutcome
    {
        using clock = std::chrono::steady_clock;
        const clock::time_point start{ clock::now() };
        FilmOutcome outcome{};

        try
        {
            const std::filesystem::path directory{ RecordingsFolder() / film };
            std::error_code error{};
            if (!std::filesystem::is_regular_file(directory / "golden.sdump", error))
            {
                outcome.Skip = "No recording in " + directory.string();
                outcome.RecordingsFailure = directory.string() + ": No recording.";
                return outcome;
            }

            std::ifstream sdump{ directory / "golden.sdump", std::ios::binary };
            SnapshotReaderState reader{};
            const std::string openReason{ SnapshotCodec::OpenReader(sdump, reader) };
            SnapshotReadResult next{ openReason.empty() ? SnapshotCodec::ReadRecord(sdump, reader) : SnapshotReadResult{} };
            if (!openReason.empty() || next.Status != SnapshotReadStatus::Record || !next.Record.LoadInfo)
            {
                outcome.Skip = openReason.empty() ? "the .sdump does not start at a map load" : openReason;
                outcome.RecordingsFailure = directory.string() + ": the .sdump cannot be read or does not start at a map load. " + openReason + next.Reason;
                return outcome;
            }

            if (!CheckMap(directory, *next.Record.LoadInfo, isCompared, outcome)) return outcome;

            std::ifstream tdump{};
            std::optional<ReplayComparer> comparer{};
            if (isCompared)
            {
                tdump.open(directory / "golden.tdump", std::ios::binary);
                comparer.emplace(tdump);
                if (comparer->Begin()) comparer->Start(next.Record.Flags);
            }

            auto chain{ std::make_unique<Chain>() };
            LoadMap(*chain, MapsFolder() / TextOf(next.Record.LoadInfo->MapFileName));

            std::unique_ptr<Chain> sparseChain{};
            std::uint32_t previousTick{ 0 };
            if (sparse.Period > 0)
            {
                sparseChain = std::make_unique<Chain>();
                LoadMap(*sparseChain, MapsFolder() / TextOf(next.Record.LoadInfo->MapFileName));
            }

            for (bool isFirst{ true }; next.Status == SnapshotReadStatus::Record; isFirst = false)
            {
                if (!isFirst && (next.Record.Flags & k_SnapshotFirstAfterLoad) != 0) break;

                ++outcome.Records;
                if ((outcome.Records - 1) % sampleEvery == 0)
                {
                    std::function<void()> endStage{};
                    if (before) endStage = before(*chain, *next.Record.Snapshot, outcome.Records);
                    chain->m_Export.m_PipelineService.Run(*next.Record.Snapshot, next.Record.Generation, endStage);
                    if (comparer) comparer->OnRecord(chain->m_Export, next.Record.DroppedBefore);
                    if (observer) observer(*chain, outcome.Records);
                    if (!observer && comparer && comparer->IsFinal()) break;
                }

                if (sparseChain)
                {
                    const std::uint32_t tick{ EngineTickOf(*next.Record.Snapshot) };
                    const bool isDecision{ tick != 0 && tick != previousTick && tick % sparse.Period == 0 };
                    previousTick = tick;

                    if (isFirst || isDecision)
                    {
                        sparseChain->m_Export.m_PipelineService.Run(*next.Record.Snapshot, next.Record.Generation, {});
                    }
                    else
                    {
                        sparseChain->m_Export.m_PipelineService.RunLight(SnapshotCodec::MakeLight(*next.Record.Snapshot));
                    }

                    if (isDecision && sparse.OnDecision) sparse.OnDecision(*chain, *sparseChain, outcome.Records);
                }

                next = SnapshotCodec::ReadRecord(sdump, reader);
            }

            if (comparer)
            {
                outcome.IsCompared = true;
                outcome.Comparison = comparer->End(next.Status, next.Reason);

                ReplayResult& result{ outcome.Comparison };
                if (result.Status == ReplayStatus::Differs || result.Status == ReplayStatus::LayoutChanged) WriteReplay(directory, result);
                // A replay.tdump left by an earlier difference would be taken for the result of this run.
                else if (result.Status == ReplayStatus::Equal) std::filesystem::remove(directory / "replay.tdump", error);

                std::vector<char>{}.swap(result.Replayed);
            }
        }
        catch (const std::exception& exception)
        {
            outcome.Error = std::format("exception in the pass of {}: {}", film, exception.what());
        }
        catch (...)
        {
            outcome.Error = std::format("unknown exception in the pass of {}", film);
        }

        outcome.Seconds = std::chrono::duration<double>(clock::now() - start).count();
        return outcome;
    }

    // return: The peak working set of the process so far, in MB (0 if the system does not say).
    auto PeakWorkingSetMegabytes() -> std::size_t
    {
        PROCESS_MEMORY_COUNTERS counters{};
        if (!GetProcessMemoryInfo(GetCurrentProcess(), &counters, sizeof(counters))) return 0;

        return counters.PeakWorkingSetSize / (1024 * 1024);
    }
}