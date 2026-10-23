export module Common.Log.System;

import Common.Log.Type;
import std;

namespace
{
    using Common::Log::Type::Level;
    using Common::Log::Type::LogSink;
}

export namespace Common::Log::System
{
    // Formats a message and sends it to a sink.
    class Log
    {
    public:
        // param sink: Must outlive the Log.
        // param tags: Origin written with every message.
        Log(LogSink& sink, std::string_view tags) : m_Sink(sink), m_Tags(tags) {}

        // Sends an Info message formatted with std::format.
        template <typename... Args>
        auto Message(std::format_string<Args...> format, Args&&... args) const -> void
        {
            this->m_Sink.Write(Level::Info, this->m_Tags, std::format(format, std::forward<Args>(args)...));
        }

    private:
        LogSink& m_Sink;
        std::string m_Tags{};
    };
}