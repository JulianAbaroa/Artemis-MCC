#include <gtest/gtest.h>
#include <gmock/gmock.h>

import Common.Log.Type;
import M0.Consumer;
import Map.Reader.Type;
import Map.Tag.Type;
import std;

namespace
{
    using Common::Log::Type::Level;
    using Common::Log::Type::LogSink;
    using Map::Tag::Type::Scnr::Structure::ScnrData;

    class NullSink : public LogSink
    {
    public:
        auto Write(Level, std::string_view, std::string_view) -> void override {}
    };
}

TEST(MapPieceTests, ScenarioSizeMatchesTheGeneratedAssertAndTheConsumerKeepsIt)
{
    EXPECT_EQ(sizeof(ScnrData), 0x858u);

    NullSink sink{};
    M0::Consumer::ScenarioHolder holder{ sink };
    EXPECT_EQ(holder.Get(), nullptr);

    ScnrData scenario{};
    scenario.StructureBsps.EntryCount = 5;
    holder.Set(scenario);

    ASSERT_NE(holder.Get(), nullptr);
    EXPECT_EQ(holder.Get()->StructureBsps.EntryCount, 5);
}