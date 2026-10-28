module Common.Log.System;

import Common.Log.Type;
import std;

namespace
{
    using Common::Log::Type::LogLevel;
    using Common::Log::Type::LogMessage;

    // Level prefixes in the order the parser tries them. The text after a prefix starts one character later (the space).
    struct LevelWord
    {
        std::string_view Word{};
        LogLevel Level{};
    };

    constexpr std::array<LevelWord, 3> k_LevelWords{ {
        { "ERROR:", LogLevel::Error },
        { "WARNING:", LogLevel::Warning },
        { "INFO:", LogLevel::Info }
    } };
}

namespace Common::Log::System
{
    auto ParseMessage(std::string_view body) -> LogMessage
    {
        LogMessage message{};

        const std::size_t tagStart = body.find('[');
        const std::size_t tagEnd = body.find(']', tagStart);

        if (tagStart != std::string_view::npos && tagEnd != std::string_view::npos)
        {
            message.Tag = std::string{ body.substr(tagStart, tagEnd - tagStart + 1) };
            body = body.substr(tagEnd + 1);

            if (!body.empty() && body[0] == ' ') body.remove_prefix(1);
        }

        for (const LevelWord& levelWord : k_LevelWords)
        {
            if (body.starts_with(levelWord.Word))
            {
                message.Level = levelWord.Level;
                body.remove_prefix(std::min(levelWord.Word.size() + 1, body.size()));
                break;
            }
        }

        message.Message = std::string{ body };
        return message;
    }

    auto LevelPrefix(LogLevel level) -> std::string_view
    {
        switch (level)
        {
        case LogLevel::Info: return "INFO: ";
        case LogLevel::Warning: return "WARNING: ";
        case LogLevel::Error: return "ERROR: ";
        default: return "";
        }
    }
}