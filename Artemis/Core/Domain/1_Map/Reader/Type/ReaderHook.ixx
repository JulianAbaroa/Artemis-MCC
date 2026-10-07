export module Map.Reader.Type:Hook;

import std;

export namespace Map::Reader::Type::Hook
{
    // Signature of the engine function that opens a map file.
    // param mapRelativePath: Null terminated path of the map, relative to the game folder.
    using OpenMapFunction = auto(__fastcall*)(std::uint64_t param1,
        std::uint64_t param2, std::uint64_t mapRelativePath, std::uint32_t* param4) -> void;
}