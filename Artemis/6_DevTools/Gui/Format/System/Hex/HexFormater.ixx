export module DevTools.Gui.Format.System:Hex;

import std;

export namespace DevTools::Gui::Format::System
{
    // Formats unsigned integers as upper-case hex, zero-padded to the width of the type, with the 0x prefix.
    class HexFormater
    {
    public:
        HexFormater() = default;
        ~HexFormater() = default;

        static auto Hex32(std::uint32_t value) -> std::string;
    };
}