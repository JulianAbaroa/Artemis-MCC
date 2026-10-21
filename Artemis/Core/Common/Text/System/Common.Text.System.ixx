export module Common.Text.System;

import std;

export namespace Common::Text::System
{
    // Escapes the text to be placed inside a JSON string.
    auto JsonEscape(std::string_view text) -> std::string;
}