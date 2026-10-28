export module Tests.Support.Log;

import Common.Log.Type;
import std;

namespace
{
    using Common::Log::Type::LogLevel;
    using Common::Log::Type::LogMessage;
    using Common::Log::Type::LogSink;
}

export namespace Tests::Support::Log
{
    // Log sink of the tests: keeps every message in memory instead of writing it anywhere.
    // note: Not thread-safe. A test that logs from several threads uses its own sink.
    class CollectingSink : public LogSink
    {
    public:
        CollectingSink() = default;
        ~CollectingSink() override = default;

        std::vector<LogMessage> m_Messages{};

        auto Write(const LogMessage& message) -> void override
        {
            m_Messages.push_back(message);
        }

        // return: How many messages have the given level.
        auto CountOf(LogLevel level) const -> std::size_t
        {
            return static_cast<std::size_t>(std::ranges::count(m_Messages, level, &LogMessage::Level));
        }
    };
}