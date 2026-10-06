export module Gui.Format.System:Hex;

import std;

export namespace Gui::Format::System
{
    // Formats unsigned integers as upper-case hex, zero-padded to the width of the type, with the 0x prefix.
    class HexFormater
    {
    public:
        HexFormater() = default;
        ~HexFormater() = default;

        static auto Hex8(std::uint8_t value) -> std::string;
        static auto Hex16(std::uint16_t value) -> std::string;
        static auto Hex32(std::uint32_t value) -> std::string;
        static auto Hex64(std::uint64_t value) -> std::string;
    };
}