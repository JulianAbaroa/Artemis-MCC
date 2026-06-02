#pragma once

#include <string>

class HexFormater
{
public:
	static std::string Hex8(uint8_t value);
	static std::string Hex16(uint16_t value);
	static std::string Hex32(uint32_t value);
	static std::string Hex64(uint64_t value);
};