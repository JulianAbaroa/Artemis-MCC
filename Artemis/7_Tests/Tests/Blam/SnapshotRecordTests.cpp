#include <gtest/gtest.h>

#include <cstddef>

import Blam.Layout.Snapshot.Type;
import Blam.Layout.Snapshot.System;
import std;

// The .sdump codec of Blam.Layout (M5c, D202, D203): a Snapshot written as a delta of bytes and read back identical, in memory.

namespace
{
    namespace Codec = Blam::Layout::Snapshot::System;

    using Blam::Layout::Snapshot::Type::RawObject;
    using Blam::Layout::Snapshot::Type::RegionId;
    using Blam::Layout::Snapshot::Type::RegionSize;
    using Blam::Layout::Snapshot::Type::Snapshot;
    using Blam::Layout::Snapshot::Type::SnapshotDumpFileHeader;
    using Blam::Layout::Snapshot::Type::SnapshotDumpFooter;
    using Blam::Layout::Snapshot::Type::SnapshotLoadInfo;
    using Blam::Layout::Snapshot::Type::SnapshotObjectBlock;
    using Blam::Layout::Snapshot::Type::SnapshotReaderState;
    using Blam::Layout::Snapshot::Type::SnapshotReadResult;
    using Blam::Layout::Snapshot::Type::SnapshotReadStatus;
    using Blam::Layout::Snapshot::Type::SnapshotRecordHeader;
    using Blam::Layout::Snapshot::Type::SnapshotRegionBlock;
    using Blam::Layout::Snapshot::Type::SnapshotSpanHeader;
    using Blam::Layout::Snapshot::Type::SnapshotWriterState;

    using Blam::Layout::Snapshot::Type::k_RegionCount;
    using Blam::Layout::Snapshot::Type::k_SnapshotDumpEndMarker;
    using Blam::Layout::Snapshot::Type::k_SnapshotFirstAfterLoad;
    using Blam::Layout::Snapshot::Type::k_SnapshotHasObjectTable;
    using Blam::Layout::Snapshot::Type::k_SnapshotHasThreadContext;
    using Blam::Layout::Snapshot::Type::k_SnapshotViewerActive;

    constexpr std::uint64_t k_LayoutHash{ 0x0123456789ABCDEF };
    constexpr std::string_view k_StartTime{ "2026-10-27 12:00:00" };

    // Fills size bytes that depend on the seed, so two different seeds never give the same bytes.
    auto Pattern(std::size_t size, std::uint8_t seed) -> std::vector<std::uint8_t>
    {
        std::vector<std::uint8_t> bytes(size);
        for (std::size_t i{ 0 }; i < size; ++i) bytes[i] = static_cast<std::uint8_t>(seed + (i * 7));
        return bytes;
    }

    // Adds a captured region. The caller adds objects in ascending index order, as the capture does.
    auto SetRegion(Snapshot& snapshot, RegionId id, std::uintptr_t address, std::size_t size, std::uint8_t seed) -> void
    {
        auto& region{ snapshot.Regions[static_cast<std::size_t>(id)] };
        region.OriginalAddress = address;
        region.Bytes = Pattern(size, seed);
    }

    auto AddObject(Snapshot& snapshot, std::uint32_t handle, std::size_t size, std::uint8_t seed, std::uint8_t kind) -> void
    {
        RawObject object{};
        object.Handle = handle;
        object.Address = 0x7FF100000000 + (static_cast<std::uintptr_t>(handle & 0xFFFF) * 0x1000);
        object.KindId = kind;
        object.Bytes = Pattern(size, seed);
        snapshot.RawObjects.emplace(handle, std::move(object));
    }

    auto ObjectOf(Snapshot& snapshot, std::uint32_t handle) -> RawObject&
    {
        return snapshot.RawObjects.at(handle);
    }

    // return: The snapshot with RawObjects inserted in ascending index order into a new map, as the capture does.
    auto AsCaptured(const Snapshot& snapshot) -> Snapshot
    {
        std::vector<const RawObject*> objects{};
        for (const auto& [handle, object] : snapshot.RawObjects) objects.push_back(&object);
        std::ranges::sort(objects, [](const RawObject* a, const RawObject* b) { return (a->Handle & 0xFFFF) < (b->Handle & 0xFFFF); });

        Snapshot captured{};
        captured.HasThreadContext = snapshot.HasThreadContext;
        captured.HasObjectTable = snapshot.HasObjectTable;
        captured.Regions = snapshot.Regions;
        for (const RawObject* object : objects) captured.RawObjects.emplace(object->Handle, *object);
        return captured;
    }

    // return: Whether the two snapshots have every field equal and iterate RawObjects in the same order, the expected one
    // being built as the capture builds it.
    auto SameSnapshot(const Snapshot& built, const Snapshot& actual) -> ::testing::AssertionResult
    {
        const Snapshot expected{ AsCaptured(built) };

        if (expected.HasThreadContext != actual.HasThreadContext) return ::testing::AssertionFailure() << "HasThreadContext differs";
        if (expected.HasObjectTable != actual.HasObjectTable) return ::testing::AssertionFailure() << "HasObjectTable differs";

        for (std::size_t i{ 0 }; i < k_RegionCount; ++i)
        {
            if (expected.Regions[i].OriginalAddress != actual.Regions[i].OriginalAddress)
            {
                return ::testing::AssertionFailure() << "region " << i << " has another address";
            }
            if (expected.Regions[i].Bytes != actual.Regions[i].Bytes) return ::testing::AssertionFailure() << "region " << i << " has other bytes";
        }

        if (expected.RawObjects.size() != actual.RawObjects.size())
        {
            return ::testing::AssertionFailure() << "object count " << actual.RawObjects.size() << " instead of " << expected.RawObjects.size();
        }

        auto left{ expected.RawObjects.begin() };
        auto right{ actual.RawObjects.begin() };
        for (; left != expected.RawObjects.end(); ++left, ++right)
        {
            if (left->first != right->first) return ::testing::AssertionFailure() << "iteration order differs at handle " << left->first;
            if (left->second.Handle != right->second.Handle || left->second.Address != right->second.Address ||
                left->second.KindId != right->second.KindId || left->second.Bytes != right->second.Bytes)
            {
                return ::testing::AssertionFailure() << "object " << left->first << " differs";
            }
        }

        return ::testing::AssertionSuccess();
    }

    // Writes a .sdump in memory and reads it back.
    class SnapshotRecordTests : public ::testing::Test
    {
    protected:
        auto WriteHeader() -> void
        {
            Codec::WriteFileHeader(m_Stream, k_LayoutHash, k_StartTime, "test");
        }

        // return: Bytes of the record.
        auto Write(const Snapshot& snapshot, std::uint32_t flags = 0, std::uint64_t generation = 1, std::uint32_t droppedBefore = 0,
            const std::optional<SnapshotLoadInfo>& loadInfo = std::nullopt) -> std::uint64_t
        {
            return Codec::WriteRecord(m_Stream, std::make_shared<const Snapshot>(snapshot), generation, droppedBefore, flags, loadInfo,
                m_WriterState);
        }

        auto WriteFooter(std::uint64_t recordedTicks) -> void
        {
            Codec::WriteFooter(m_Stream, SnapshotDumpFooter{ .Marker = k_SnapshotDumpEndMarker, .RecordedTicks = recordedTicks });
        }

        std::stringstream m_Stream{};
        SnapshotWriterState m_WriterState{};
        SnapshotReaderState m_Reader{};
    };
}

TEST_F(SnapshotRecordTests, RoundTripEmpty)
{
    this->WriteHeader();
    this->Write(Snapshot{});
    this->WriteFooter(1);

    std::istringstream in{ m_Stream.str() };
    ASSERT_EQ(Codec::OpenReader(in, m_Reader), "");

    const SnapshotReadResult record{ Codec::ReadRecord(in, m_Reader) };
    ASSERT_EQ(record.Status, SnapshotReadStatus::Record);
    ASSERT_NE(record.Record.Snapshot, nullptr);
    EXPECT_TRUE(SameSnapshot(Snapshot{}, *record.Record.Snapshot));
    EXPECT_EQ(record.Record.Generation, 1u);
    EXPECT_FALSE(record.Record.LoadInfo.has_value());

    EXPECT_EQ(Codec::ReadRecord(in, m_Reader).Status, SnapshotReadStatus::End);
    ASSERT_TRUE(m_Reader.Footer.has_value());
    EXPECT_EQ(m_Reader.Footer->RecordedTicks, 1u);
}

TEST_F(SnapshotRecordTests, RoundTripFull)
{
    Snapshot snapshot{};
    snapshot.HasThreadContext = true;
    snapshot.HasObjectTable = true;
    SetRegion(snapshot, RegionId::PlayerArray, 0x7FF000001000, RegionSize(RegionId::PlayerArray), 1);
    SetRegion(snapshot, RegionId::TimeGlobals, 0x7FF000002000, RegionSize(RegionId::TimeGlobals), 2);
    SetRegion(snapshot, RegionId::Placements, 0x7FF000003000, 200, 3);
    AddObject(snapshot, 0xE001'0002, 40, 10, 1);
    AddObject(snapshot, 0xE002'0005, 1000, 20, 3);
    AddObject(snapshot, 0xE003'0009, 7, 30, 6);

    // The same objects inserted in ascending index order, as CopyObjects does, to compare the iteration order.
    Snapshot expected{};
    expected.HasThreadContext = true;
    expected.HasObjectTable = true;
    expected.Regions = snapshot.Regions;
    for (const std::uint32_t handle : { 0xE001'0002u, 0xE002'0005u, 0xE003'0009u })
    {
        expected.RawObjects.emplace(handle, snapshot.RawObjects.at(handle));
    }

    SnapshotLoadInfo info{ .MapFileSize = 332328960 };
    std::string_view name{ "forge_halo.map" };
    std::ranges::copy(name, info.MapFileName.begin());

    this->WriteHeader();
    this->Write(snapshot, k_SnapshotFirstAfterLoad | k_SnapshotViewerActive, 0x1122334455667788, 7, info);
    this->WriteFooter(1);

    std::istringstream in{ m_Stream.str() };
    ASSERT_EQ(Codec::OpenReader(in, m_Reader), "");
    EXPECT_EQ(m_Reader.Header.BridgeLayoutHash, k_LayoutHash);

    const SnapshotReadResult record{ Codec::ReadRecord(in, m_Reader) };
    ASSERT_EQ(record.Status, SnapshotReadStatus::Record);
    EXPECT_TRUE(SameSnapshot(expected, *record.Record.Snapshot));
    EXPECT_EQ(record.Record.Generation, 0x1122334455667788u);
    EXPECT_EQ(record.Record.DroppedBefore, 7u);
    EXPECT_EQ(record.Record.Flags, k_SnapshotFirstAfterLoad | k_SnapshotViewerActive | k_SnapshotHasThreadContext | k_SnapshotHasObjectTable);
    ASSERT_TRUE(record.Record.LoadInfo.has_value());
    EXPECT_EQ(record.Record.LoadInfo->MapFileSize, 332328960u);
    EXPECT_EQ(record.Record.LoadInfo->MapFileName, info.MapFileName);
}

TEST_F(SnapshotRecordTests, DeltaSequence)
{
    std::vector<Snapshot> sequence{};

    // 0: the first record, everything is new.
    Snapshot base{};
    base.HasThreadContext = true;
    base.HasObjectTable = true;
    SetRegion(base, RegionId::PlayerArray, 0x7FF000001000, 300, 1);
    SetRegion(base, RegionId::EventGlobals, 0x7FF000002000, 80, 2);
    AddObject(base, 0xE001'0002, 64, 10, 1);
    AddObject(base, 0xE002'0005, 32, 20, 2);
    AddObject(base, 0xE003'0009, 100, 30, 3);
    sequence.push_back(base);

    // 1: equal to the previous one.
    sequence.push_back(sequence.back());

    // 2: one object changes 4 bytes in two places.
    Snapshot changed{ sequence.back() };
    changed.RawObjects.at(0xE001'0002).Bytes[4] ^= 0x5A;
    changed.RawObjects.at(0xE001'0002).Bytes[5] ^= 0x5A;
    changed.RawObjects.at(0xE001'0002).Bytes[50] ^= 0x33;
    changed.RawObjects.at(0xE001'0002).Bytes[51] ^= 0x33;
    sequence.push_back(changed);

    // 3: an object is deleted and another one enters, with an index below the ones that stay.
    Snapshot swapped{};
    swapped.HasThreadContext = true;
    swapped.HasObjectTable = true;
    swapped.Regions = changed.Regions;
    AddObject(swapped, 0xE004'0001, 48, 40, 4);
    swapped.RawObjects.emplace(0xE001'0002, changed.RawObjects.at(0xE001'0002));
    swapped.RawObjects.emplace(0xE003'0009, changed.RawObjects.at(0xE003'0009));
    sequence.push_back(swapped);

    // 4: a region is not captured anymore and another one moves in memory.
    Snapshot moved{ sequence.back() };
    moved.Regions[static_cast<std::size_t>(RegionId::PlayerArray)].Bytes.clear();
    moved.Regions[static_cast<std::size_t>(RegionId::EventGlobals)].OriginalAddress = 0x7FF000009000;
    sequence.push_back(moved);

    this->WriteHeader();
    for (std::size_t i{ 0 }; i < sequence.size(); ++i) this->Write(sequence[i], 0, i + 1);
    this->WriteFooter(sequence.size());

    std::istringstream in{ m_Stream.str() };
    ASSERT_EQ(Codec::OpenReader(in, m_Reader), "");
    for (std::size_t i{ 0 }; i < sequence.size(); ++i)
    {
        const SnapshotReadResult record{ Codec::ReadRecord(in, m_Reader) };
        ASSERT_EQ(record.Status, SnapshotReadStatus::Record) << record.Reason;
        EXPECT_EQ(record.Record.Generation, i + 1);
        EXPECT_TRUE(SameSnapshot(sequence[i], *record.Record.Snapshot)) << "record " << i;
    }
    EXPECT_EQ(Codec::ReadRecord(in, m_Reader).Status, SnapshotReadStatus::End);
}

TEST_F(SnapshotRecordTests, UnchangedRecordIsSmall)
{
    Snapshot snapshot{};
    snapshot.HasThreadContext = true;
    SetRegion(snapshot, RegionId::PlayerArray, 0x7FF000001000, 500, 1);
    AddObject(snapshot, 0xE001'0002, 400, 10, 1);

    this->WriteHeader();
    EXPECT_GT(this->Write(snapshot), sizeof(SnapshotRecordHeader));

    const std::streamoff before{ m_Stream.tellp() };
    const std::uint64_t bytes{ this->Write(snapshot) };

    EXPECT_EQ(bytes, sizeof(SnapshotRecordHeader));
    EXPECT_EQ(static_cast<std::uint64_t>(m_Stream.tellp() - before), sizeof(SnapshotRecordHeader));
}

TEST_F(SnapshotRecordTests, SpansMerge)
{
    Snapshot base{};
    SetRegion(base, RegionId::PlayerArray, 0x7FF000001000, 100, 0);
    for (std::uint8_t& byte : base.Regions[static_cast<std::size_t>(RegionId::PlayerArray)].Bytes) byte = 0;

    // Bytes 10 and 26 have 15 equal bytes between them: one span of 17 bytes.
    Snapshot joined{ base };
    joined.Regions[static_cast<std::size_t>(RegionId::PlayerArray)].Bytes[10] = 1;
    joined.Regions[static_cast<std::size_t>(RegionId::PlayerArray)].Bytes[26] = 1;

    // Bytes 10 and 27 have 16 equal bytes between them: two spans of one byte.
    Snapshot separate{ base };
    separate.Regions[static_cast<std::size_t>(RegionId::PlayerArray)].Bytes[10] = 1;
    separate.Regions[static_cast<std::size_t>(RegionId::PlayerArray)].Bytes[27] = 1;

    const std::uint64_t fixed{ sizeof(SnapshotRecordHeader) + sizeof(SnapshotRegionBlock) };

    this->WriteHeader();
    this->Write(base);
    EXPECT_EQ(this->Write(joined), fixed + sizeof(SnapshotSpanHeader) + 17);

    // Back to the base, then the separate change.
    this->Write(base);
    EXPECT_EQ(this->Write(separate), fixed + (2 * (sizeof(SnapshotSpanHeader) + 1)));

    std::istringstream in{ m_Stream.str() };
    ASSERT_EQ(Codec::OpenReader(in, m_Reader), "");
    EXPECT_TRUE(SameSnapshot(base, *Codec::ReadRecord(in, m_Reader).Record.Snapshot));
    EXPECT_TRUE(SameSnapshot(joined, *Codec::ReadRecord(in, m_Reader).Record.Snapshot));
    EXPECT_TRUE(SameSnapshot(base, *Codec::ReadRecord(in, m_Reader).Record.Snapshot));
    EXPECT_TRUE(SameSnapshot(separate, *Codec::ReadRecord(in, m_Reader).Record.Snapshot));
}

TEST_F(SnapshotRecordTests, CutFileIsReported)
{
    Snapshot first{};
    SetRegion(first, RegionId::PlayerArray, 0x7FF000001000, 200, 1);
    AddObject(first, 0xE001'0002, 100, 10, 1);

    Snapshot second{ first };
    second.RawObjects.at(0xE001'0002).Bytes[3] ^= 0x7F;

    Snapshot third{ second };
    AddObject(third, 0xE002'0005, 300, 20, 2);

    this->WriteHeader();
    this->Write(first);
    this->Write(second);
    const std::size_t secondEnd{ static_cast<std::size_t>(m_Stream.tellp()) };
    this->Write(third);
    const std::size_t thirdEnd{ static_cast<std::size_t>(m_Stream.tellp()) };
    this->WriteFooter(3);
    const std::string whole{ m_Stream.str() };

    // Cut in the middle of the third record.
    {
        std::istringstream in{ whole.substr(0, secondEnd + ((thirdEnd - secondEnd) / 2)) };
        ASSERT_EQ(Codec::OpenReader(in, m_Reader), "");
        EXPECT_TRUE(SameSnapshot(first, *Codec::ReadRecord(in, m_Reader).Record.Snapshot));
        EXPECT_TRUE(SameSnapshot(second, *Codec::ReadRecord(in, m_Reader).Record.Snapshot));

        const SnapshotReadResult cut{ Codec::ReadRecord(in, m_Reader) };
        EXPECT_EQ(cut.Status, SnapshotReadStatus::Cut);
        EXPECT_FALSE(cut.Reason.empty());
    }

    // Complete records and no footer.
    {
        SnapshotReaderState reader{};
        std::istringstream in{ whole.substr(0, thirdEnd) };
        ASSERT_EQ(Codec::OpenReader(in, reader), "");
        for (int i{ 0 }; i < 3; ++i) EXPECT_EQ(Codec::ReadRecord(in, reader).Status, SnapshotReadStatus::Record);

        const SnapshotReadResult cut{ Codec::ReadRecord(in, reader) };
        EXPECT_EQ(cut.Status, SnapshotReadStatus::Cut);
        EXPECT_FALSE(reader.Footer.has_value());
    }

    // Cut inside the footer.
    {
        SnapshotReaderState reader{};
        std::istringstream in{ whole.substr(0, whole.size() - 10) };
        ASSERT_EQ(Codec::OpenReader(in, reader), "");
        for (int i{ 0 }; i < 3; ++i) EXPECT_EQ(Codec::ReadRecord(in, reader).Status, SnapshotReadStatus::Record);
        EXPECT_EQ(Codec::ReadRecord(in, reader).Status, SnapshotReadStatus::Cut);
    }
}

TEST_F(SnapshotRecordTests, LayoutMismatchIsRefused)
{
    Snapshot snapshot{};
    SetRegion(snapshot, RegionId::PlayerArray, 0x7FF000001000, 64, 1);

    this->WriteHeader();
    this->Write(snapshot);
    this->WriteFooter(1);

    // Another size for region 2, as if the layout of that region had changed.
    std::string bytes{ m_Stream.str() };
    const std::size_t offset{ offsetof(SnapshotDumpFileHeader, RegionSizes) + (2 * sizeof(std::uint32_t)) };
    std::uint32_t size{};
    std::memcpy(&size, bytes.data() + offset, sizeof(size));
    ++size;
    std::memcpy(bytes.data() + offset, &size, sizeof(size));

    std::istringstream in{ bytes };
    const std::string reason{ Codec::OpenReader(in, m_Reader) };

    EXPECT_NE(reason, "");
    EXPECT_NE(reason.find("region 2"), std::string::npos) << reason;
    EXPECT_EQ(Codec::ReadRecord(in, m_Reader).Status, SnapshotReadStatus::Error);
}