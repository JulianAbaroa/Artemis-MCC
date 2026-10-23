export module Common.Log.System;

import Common.Log.Type;
import std;

namespace
{
    using Common::Log::Type::LogLevel;
    using Common::Log::Type::LogMessage;
    using Common::Log::Type::LogSink;
}

export namespace Common::Log::System
{
    // Splits a formatted body into tag, level and text.
    // note: Body syntax is "[Tag] LEVEL: text". Tag and LEVEL (ERROR, WARNING, INFO) are optional. The first "[...]" pair anywhere
    // is the tag (the text before it is dropped), and one space after the tag and one after the level are dropped (D166).
    auto ParseMessage(std::string_view body) -> LogMessage;

    // return: The text a level adds before the message: "", "INFO: ", "WARNING: " or "ERROR: ".
    auto LevelPrefix(LogLevel level) -> std::string_view;

    // Formats a message, parses it and sends it to the sink.
    // note: Has no mutable state, so it is thread-safe whenever the sink is.
    class Log
    {
    public:
        // param sink: Must outlive the Log.
        explicit Log(LogSink& sink) : m_Sink(sink) {}
        ~Log() = default;

        Log(const Log&) = delete;
        auto operator=(const Log&) -> Log& = delete;

        // Formats a message and writes it to the sink once.
        template <typename... Args>
        auto Message(std::format_string<Args...> fmt, Args&&... args) -> void
        {
            m_Sink.Write(ParseMessage(std::format(fmt, std::forward<Args>(args)...)));
        }

    private:
        LogSink& m_Sink;
    };
}