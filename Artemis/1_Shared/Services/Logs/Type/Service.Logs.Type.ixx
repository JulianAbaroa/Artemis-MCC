export module Service.Logs.Type;

import Common.Log.Type;
import std;

namespace
{
    using Common::Log::Type::LogLevel;
}

export namespace Service::Logs::Type
{
    // One parsed log line. Source syntax is "[Tag] LEVEL: text", with Tag and LEVEL optional.
    struct LogEntry
    {
        // Timestamp, tag, prefix and message joined. This is what gets displayed and written to file.
        std::string FullText{};
        std::string Timestamp{};

        // Includes the brackets.
        std::string Tag{};

        // Level text with trailing space, such as "INFO: ".
        std::string MessagePrefix{};
        std::string Message{};
        LogLevel Level{};

        // Number of the entry since Artemis started, from 1. Set by LogsStore::PushBack.
        std::uint64_t Sequence{ 0 };
    };
}