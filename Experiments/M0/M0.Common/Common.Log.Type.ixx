export module Common.Log.Type;

import std;

export namespace Common::Log::Type
{
    // Severity of a log message.
    enum class Level : std::uint8_t
    {
        Info,
        Warning,
        Error,
    };

    // Destination of log messages. The tests replace it with a mock.
    class LogSink
    {
    public:
        LogSink() = default;
        virtual ~LogSink() = default;

        // param tags: Origin of the message, e.g. "[Consumer]".
        // param text: Message already formatted.
        virtual auto Write(Level level, std::string_view tags, std::string_view text) -> void = 0;
    };
}