#include "pch.h"

#include "HexFormater.h"

#include <format>

std::string HexFormater::Hex8(uint8_t value)
{
	return std::format("0x{:02X}", value);
}

std::string HexFormater::Hex16(uint16_t value)
{
	return std::format("0x{:04X}", value);
}

std::string HexFormater::Hex32(uint32_t value)
{
	return std::format("0x{:08X}", value);
}

std::string HexFormater::Hex64(uint64_t value)
{
	return std::format("0x{:016X}", value);
}