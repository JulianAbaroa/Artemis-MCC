module Common.Text.System;

import std;

namespace Common::Text::System
{
    auto JsonEscape(std::string_view text) -> std::string
    {
        std::string out{};
        out.reserve(text.size() + 2);
        for (const char c : text)
        {
            switch (c)
            {
            case '\\': out += "\\\\"; break;
            case '"': out += "\\\""; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < ' ')
                {
                    out += std::format("\\u{:04x}", static_cast<int>(c));
                }
                else
                {
                    out += c;
                }
            }
        }
        return out;
    }
}