export module Common.Log.Type;

import std;

export namespace Common::Log::Type
{
    // Severity of a log message. Default means the message had no level prefix.
    enum class LogLevel
    {
        Default,
        Info,
        Warning,
        Error
    };

    // One parsed log message. Source syntax is "[Tag] LEVEL: text", with Tag and LEVEL optional.
    struct LogMessage
    {
        LogLevel Level{};

        // Includes the brackets, or empty.
        std::string Tag{};

        // Text without the level prefix.
        std::string Message{};

        auto operator==(const LogMessage&) const -> bool = default;
    };

    // Destination of the log messages. The Services layer implements the real one.
    // note: Write can be called from any thread. The sink decides how to synchronize.
    class LogSink
    {
    public:
        virtual ~LogSink() = default;

        virtual auto Write(const LogMessage& message) -> void = 0;
    };
}