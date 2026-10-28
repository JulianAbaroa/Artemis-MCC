export module Resolved.Tag.Type;

import std;

export namespace Resolved::Tag::Type
{
    // Name and group of a tag, resolved from a handle.
    struct TagInfo
    {
        bool IsValid{ false };
        std::string FourCC{};
        std::string TagName{};
    };
}