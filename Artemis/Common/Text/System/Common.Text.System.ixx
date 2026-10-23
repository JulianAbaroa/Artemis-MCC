export module Common.Text.System;

import std;

export namespace Common::Text::System
{
    // Bytes in a megabyte, to show file sizes.
    inline constexpr double k_BytesPerMegabyte{ 1024.0 * 1024.0 };

    // Escapes the text to be placed inside a JSON string.
    auto JsonEscape(std::string_view text) -> std::string;

    // Copies text into a zero padded buffer, cut to leave the last byte as terminator.
    template <std::size_t N>
    auto CopyText(std::string_view text, std::array<char, N>& out) -> void
    {
        const std::size_t length{ (std::min)(text.size(), N - 1) };
        std::copy_n(text.data(), length, out.begin());
    }
}