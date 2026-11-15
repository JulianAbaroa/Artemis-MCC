module DevTools.Gui.Format.System;
import :Hex;
import std;

namespace DevTools::Gui::Format::System
{
    auto HexFormater::Hex32(std::uint32_t value) -> std::string
    {
        return std::format("0x{:08X}", value);
    }
}