module UI.Format.System;
import :Hex;

namespace UI::Format::System
{
	auto HexFormater::Hex8(std::uint8_t value) -> std::string
	{
		return std::format("0x{:02X}", value);
	}

	auto HexFormater::Hex16(std::uint16_t value) -> std::string
	{
		return std::format("0x{:04X}", value);
	}

	auto HexFormater::Hex32(std::uint32_t value) -> std::string
	{
		return std::format("0x{:08X}", value);
	}

	auto HexFormater::Hex64(std::uint64_t value) -> std::string
	{
		return std::format("0x{:016X}", value);
	}
}