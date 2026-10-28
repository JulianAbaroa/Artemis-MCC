#include <gtest/gtest.h>
#include <gmock/gmock.h>

import Common.Log.Type;
import Common.Log.System;
import Tests.Support.Log;
import std;

namespace
{
    using Common::Log::Type::LogLevel;
    using Common::Log::Type::LogMessage;
    using Common::Log::Type::LogSink;
    using Common::Log::System::Log;
    using Tests::Support::Log::CollectingSink;

    class MockSink : public LogSink
    {
    public:
        MOCK_METHOD(void, Write, (const LogMessage& message), (override));
    };
}

TEST(LogTests, MessageFormatsParsesAndWritesOnce)
{
    MockSink sink{};
    const LogMessage expected{ .Level = LogLevel::Error, .Tag = "[Test]", .Message = "Failed 3 times on halo." };
    EXPECT_CALL(sink, Write(expected)).Times(1);

    Log log{ sink };
    log.Message("[Test] ERROR: Failed {} times on {}.", 3, "halo");
}

TEST(LogTests, EachMessageReachesTheSinkInOrder)
{
    CollectingSink sink{};
    Log log{ sink };

    log.Message("[A] INFO: one.");
    log.Message("[B] WARNING: two.");
    log.Message("three");

    ASSERT_EQ(sink.m_Messages.size(), 3u);
    EXPECT_EQ(sink.m_Messages[0].Tag, "[A]");
    EXPECT_EQ(sink.m_Messages[1].Level, LogLevel::Warning);
    EXPECT_EQ(sink.m_Messages[2].Message, "three");
    EXPECT_EQ(sink.CountOf(LogLevel::Error), 0u);
}

TEST(LogTests, LogIsNotCopyable)
{
    static_assert(!std::is_copy_constructible_v<Log>);
    static_assert(!std::is_copy_assignable_v<Log>);
}