#include <gtest/gtest.h>
#include <gmock/gmock.h>

import Common.Log.System;
import Common.Log.Type;
import std;

namespace
{
    using Common::Log::System::Log;
    using Common::Log::Type::Level;
    using Common::Log::Type::LogSink;

    class MockSink : public LogSink
    {
    public:
        MOCK_METHOD(void, Write, (Level level, std::string_view tags, std::string_view text), (override));
    };
}

TEST(LogTests, MessageCallsWriteOnceWithTheFormattedText)
{
    MockSink sink{};
    EXPECT_CALL(sink, Write(Level::Info, std::string_view{ "[Test]" }, std::string_view{ "Loaded 3 maps from halo." })).Times(1);

    Log log{ sink, "[Test]" };
    log.Message("Loaded {} maps from {}.", 3, "halo");
}