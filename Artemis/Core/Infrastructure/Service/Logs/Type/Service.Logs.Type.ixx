export module Service.Logs.Type;

import std;

export namespace Service::Logs::Type
{
    // Severity of a log entry. Default means the message had no level prefix.
    enum class LogLevel
    {
        Default,
        Info,
        Warning,
        Error
    };

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
    };
}