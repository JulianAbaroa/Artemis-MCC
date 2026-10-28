module Common.Text.System;

import std;

namespace
{
    // UTF-8 encoding: first code point that needs 2, 3 and 4 bytes, the lead byte marks and the continuation byte.
    constexpr char32_t k_TwoBytes{ 0x80 };
    constexpr char32_t k_ThreeBytes{ 0x800 };
    constexpr char32_t k_FourBytes{ 0x10000 };
    constexpr char32_t k_LeadTwo{ 0xC0 };
    constexpr char32_t k_LeadThree{ 0xE0 };
    constexpr char32_t k_LeadFour{ 0xF0 };
    constexpr char32_t k_Continuation{ 0x80 };
    constexpr char32_t k_ContinuationPayload{ 0x3F };
    constexpr std::size_t k_ContinuationBits{ 6 };

    // UTF-16 surrogates: high ones lead a pair, low ones close it. The 10 bits of each half make the offset over the BMP.
    constexpr char32_t k_HighSurrogateFirst{ 0xD800 };
    constexpr char32_t k_LowSurrogateFirst{ 0xDC00 };
    constexpr char32_t k_SurrogateLast{ 0xDFFF };
    constexpr std::size_t k_SurrogateBits{ 10 };

    // U+FFFD, what stands for a lone surrogate.
    constexpr char32_t k_Replacement{ 0xFFFD };

    // return: The continuation byte of the six bits of the code point that start at shift.
    auto Continue(char32_t codePoint, std::size_t shift) -> char
    {
        return static_cast<char>(k_Continuation | ((codePoint >> shift) & k_ContinuationPayload));
    }

    // Appends the UTF-8 bytes of a code point.
    auto AppendUtf8(std::string& out, char32_t codePoint) -> void
    {
        if (codePoint < k_TwoBytes)
        {
            out += static_cast<char>(codePoint);
        }
        else if (codePoint < k_ThreeBytes)
        {
            out += static_cast<char>(k_LeadTwo | (codePoint >> k_ContinuationBits));
            out += Continue(codePoint, 0);
        }
        else if (codePoint < k_FourBytes)
        {
            out += static_cast<char>(k_LeadThree | (codePoint >> (2 * k_ContinuationBits)));
            out += Continue(codePoint, k_ContinuationBits);
            out += Continue(codePoint, 0);
        }
        else
        {
            out += static_cast<char>(k_LeadFour | (codePoint >> (3 * k_ContinuationBits)));
            out += Continue(codePoint, 2 * k_ContinuationBits);
            out += Continue(codePoint, k_ContinuationBits);
            out += Continue(codePoint, 0);
        }
    }
}

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

    auto WideToUtf8(std::span<const char16_t> source) -> std::string
    {
        std::string out{};
        out.reserve(source.size() * 3);

        for (std::size_t i{ 0 }; i < source.size() && source[i] != 0; ++i)
        {
            const char32_t unit{ source[i] };

            if (unit < k_HighSurrogateFirst || unit > k_SurrogateLast)
            {
                AppendUtf8(out, unit);
            }
            else if (unit < k_LowSurrogateFirst && i + 1 < source.size() && source[i + 1] >= k_LowSurrogateFirst && source[i + 1] <= k_SurrogateLast)
            {
                AppendUtf8(out, k_FourBytes + ((unit - k_HighSurrogateFirst) << k_SurrogateBits) + (source[i + 1] - k_LowSurrogateFirst));
                ++i;
            }
            else
            {
                AppendUtf8(out, k_Replacement);
            }
        }

        return out;
    }
}