export module UI.Format.System:Hex;

import std;

export namespace UI::Format::System
{
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