#include <gtest/gtest.h>

import Contract.Bridge;
import Export.Dump.Type;
import Export.Dump.System;
import std;

// The writer of the .tdump (M5c, D203): the delta of the entities and of the events between two records, and the sizes of the
// blocks, read back with a reader written here from the sizes of the layout.

namespace
{
    namespace Writer = Export::Dump::System;

    using Platform::Bridge::Type::BridgeEntity;
    using Platform::Bridge::Type::BridgeEvent;
    using Platform::Bridge::Type::BridgeMatch;
    using Platform::Bridge::Type::BridgePlayer;
    using Platform::Bridge::Type::BridgeTick;
    using Platform::Bridge::Type::BridgeTickHeader;

    using Export::Dump::Type::TickDumpCounters;
    using Export::Dump::Type::TickDumpFileHeader;
    using Export::Dump::Type::TickDumpFooter;
    using Export::Dump::Type::TickDumpWriterState;
    using Export::Dump::Type::TickRecordHeader;

    using Platform::Bridge::Type::k_BridgeLayoutJson;
    using Export::Dump::Type::k_TickDumpEndMarker;
    using Export::Dump::Type::k_TickRecordEventsLost;
    using Export::Dump::Type::k_TickRecordHasMatch;
    using Export::Dump::Type::k_TickRecordKeyframe;
    using Export::Dump::Type::k_TickRecordMarker;

    // One record read back.
    struct ParsedRecord
    {
        TickRecordHeader Header{};
        BridgeTickHeader Tick{};
        BridgeMatch Match{};
        std::vector<BridgePlayer> Players{};
        std::vector<BridgeEntity> Entities{};
        std::vector<std::uint32_t> Removed{};
        std::vector<BridgeEvent> Events{};
    };

    template <typename T>
    auto ReadValue(std::istream& stream, T& value) -> void
    {
        stream.read(reinterpret_cast<char*>(&value), sizeof(T));
    }

    template <typename T>
    auto ReadRows(std::istream& stream, std::size_t count) -> std::vector<T>
    {
        std::vector<T> rows(count);
        if (count != 0) stream.read(reinterpret_cast<char*>(rows.data()), static_cast<std::streamsize>(count * sizeof(T)));
        return rows;
    }

    // Reads one record with the sizes of the layout, as a reader of the file does.
    auto ReadRecord(std::istream& stream) -> ParsedRecord
    {
        ParsedRecord record{};
        ReadValue(stream, record.Header);
        ReadValue(stream, record.Tick);
        ReadValue(stream, record.Match);
        record.Players = ReadRows<BridgePlayer>(stream, record.Tick.PlayerCount);
        record.Entities = ReadRows<BridgeEntity>(stream, record.Header.ChangedEntityCount);
        record.Removed = ReadRows<std::uint32_t>(stream, record.Header.RemovedEntityCount);
        record.Events = ReadRows<BridgeEvent>(stream, record.Header.NewEventCount);
        return record;
    }

    auto MakeEntity(std::uint32_t handle, float vitality) -> BridgeEntity
    {
        BridgeEntity entity{};
        entity.Handle = handle;
        entity.Vitality = vitality;
        return entity;
    }

    auto MakeEvent(std::uint32_t sequence) -> BridgeEvent
    {
        BridgeEvent event{};
        event.Sequence = sequence;
        event.GameTick = 100 + sequence;
        return event;
    }

    // return: A BridgeTick with the entities (sorted by handle), a player and the events, as Export.Flat leaves it.
    auto MakeTick(std::span<const BridgeEntity> entities, std::span<const BridgeEvent> events, std::uint32_t gameTick) -> std::unique_ptr<BridgeTick>
    {
        auto flat{ std::make_unique<BridgeTick>() };
        std::ranges::copy(entities, flat->Entities.begin());
        std::ranges::copy(events, flat->Events.begin());
        flat->Players[0].Handle = 0xE5A1'0002;
        flat->Match.GameTick = gameTick;
        flat->Header.PlayerCount = 1;
        flat->Header.EntityCount = static_cast<std::uint16_t>(entities.size());
        flat->Header.EventCount = static_cast<std::uint16_t>(events.size());
        flat->Header.LastEventSequence = events.empty() ? 0 : events.back().Sequence;
        return flat;
    }
}

TEST(TickDumpWriterTests, FileHeaderAndFooterHaveTheSizesOfTheLayout)
{
    std::stringstream stream{};
    const std::uint64_t header{ Writer::WriteFileHeader(stream, "forge_halo", "2026-10-27 12:00:00", "manual") };
    EXPECT_EQ(header, sizeof(TickDumpFileHeader) + k_BridgeLayoutJson.size());
    EXPECT_EQ(static_cast<std::uint64_t>(stream.tellp()), header);

    const std::uint64_t footer{ Writer::WriteFooter(stream, TickDumpCounters{ .RecordedTicks = 5 }) };
    EXPECT_EQ(footer, sizeof(TickDumpFooter));
    EXPECT_EQ(sizeof(TickDumpFooter), 40u);

    const std::string bytes{ stream.str() };
    TickDumpFileHeader read{};
    std::memcpy(&read, bytes.data(), sizeof(read));
    EXPECT_EQ(read.HeaderSize, sizeof(TickDumpFileHeader));
    EXPECT_EQ(read.LayoutJsonSize, k_BridgeLayoutJson.size());
    EXPECT_EQ(read.RecordHeaderSize, sizeof(TickRecordHeader));
    const std::string_view json{ bytes.data() + sizeof(read), read.LayoutJsonSize };
    EXPECT_EQ(json, k_BridgeLayoutJson);
}

TEST(TickDumpWriterTests, FirstRecordIsAKeyframeAndTheNextOneIsADelta)
{
    TickDumpWriterState state{};
    Writer::Begin(state);
    std::stringstream stream{};

    const std::array<BridgeEntity, 3> first{ MakeEntity(10, 1.0f), MakeEntity(20, 2.0f), MakeEntity(30, 3.0f) };
    const std::array<BridgeEvent, 2> firstEvents{ MakeEvent(1), MakeEvent(2) };
    const auto tickOne{ MakeTick(first, firstEvents, 100) };
    const auto one{ Writer::WriteRecord(stream, *tickOne, true, 0, state) };
    EXPECT_FALSE(one.IsEventsLost);

    // The entity 20 changes, 30 is gone and 40 is new. The window of events gains the sequence 3.
    const std::array<BridgeEntity, 3> second{ MakeEntity(10, 1.0f), MakeEntity(20, 2.5f), MakeEntity(40, 4.0f) };
    const std::array<BridgeEvent, 3> secondEvents{ MakeEvent(1), MakeEvent(2), MakeEvent(3) };
    const auto tickTwo{ MakeTick(second, secondEvents, 101) };
    const auto two{ Writer::WriteRecord(stream, *tickTwo, true, 3, state) };
    EXPECT_FALSE(two.IsEventsLost);

    EXPECT_EQ(one.Bytes + two.Bytes, static_cast<std::uint64_t>(stream.tellp()));

    std::istringstream in{ stream.str() };

    const ParsedRecord keyframe{ ReadRecord(in) };
    EXPECT_EQ(keyframe.Header.Marker, k_TickRecordMarker);
    EXPECT_EQ(keyframe.Header.Flags, k_TickRecordHasMatch | k_TickRecordKeyframe);
    EXPECT_EQ(keyframe.Header.GameTick, 100u);
    ASSERT_EQ(keyframe.Entities.size(), 3u);
    EXPECT_EQ(keyframe.Entities[0].Handle, 10u);
    EXPECT_EQ(keyframe.Entities[2].Handle, 30u);
    EXPECT_TRUE(keyframe.Removed.empty());
    EXPECT_EQ(keyframe.Tick.EntityCount, 3u);
    ASSERT_EQ(keyframe.Events.size(), 2u);
    ASSERT_EQ(keyframe.Players.size(), 1u);

    const ParsedRecord delta{ ReadRecord(in) };
    EXPECT_EQ(delta.Header.Flags, k_TickRecordHasMatch);
    EXPECT_EQ(delta.Header.DroppedBefore, 3u);
    EXPECT_EQ(delta.Header.GameTick, 101u);
    ASSERT_EQ(delta.Entities.size(), 2u);
    EXPECT_EQ(delta.Entities[0].Handle, 20u);
    EXPECT_EQ(delta.Entities[0].Vitality, 2.5f);
    EXPECT_EQ(delta.Entities[1].Handle, 40u);
    ASSERT_EQ(delta.Removed.size(), 1u);
    EXPECT_EQ(delta.Removed[0], 30u);
    // The count of the header is the live count either way.
    EXPECT_EQ(delta.Tick.EntityCount, 3u);

    // Only the event that was not written yet.
    ASSERT_EQ(delta.Events.size(), 1u);
    EXPECT_EQ(delta.Events[0].Sequence, 3u);

    // Nothing is left after the second record.
    in.peek();
    EXPECT_TRUE(in.eof());
}

TEST(TickDumpWriterTests, ASequenceJumpIsReportedAsLostEvents)
{
    TickDumpWriterState state{};
    Writer::Begin(state);
    std::stringstream stream{};

    const std::array<BridgeEvent, 2> firstEvents{ MakeEvent(1), MakeEvent(2) };
    Writer::WriteRecord(stream, *MakeTick({}, firstEvents, 1), true, 0, state);

    // Events 3 to 5 left the window before they were written.
    const std::array<BridgeEvent, 1> lateEvents{ MakeEvent(6) };
    const auto lost{ Writer::WriteRecord(stream, *MakeTick({}, lateEvents, 2), true, 0, state) };
    EXPECT_TRUE(lost.IsEventsLost);

    std::istringstream in{ stream.str() };
    ReadRecord(in);
    const ParsedRecord second{ ReadRecord(in) };
    EXPECT_TRUE((second.Header.Flags & k_TickRecordEventsLost) != 0);
    ASSERT_EQ(second.Events.size(), 1u);
    EXPECT_EQ(second.Events[0].Sequence, 6u);
}

TEST(TickDumpWriterTests, ARecordWithoutMatchHasNoGameTick)
{
    TickDumpWriterState state{};
    Writer::Begin(state);
    std::stringstream stream{};

    Writer::WriteRecord(stream, *MakeTick({}, {}, 0), false, 0, state);

    std::istringstream in{ stream.str() };
    const ParsedRecord record{ ReadRecord(in) };
    EXPECT_FALSE((record.Header.Flags & k_TickRecordHasMatch) != 0);
    EXPECT_EQ(record.Header.GameTick, 0u);
}