export module Map.Reader.Type:Magic;

import std;

export namespace Map::Reader::Type::Magic
{
	constexpr std::int64_t k_Expand{ 0x50000000LL };

	constexpr std::uint32_t k_IndexHeader{ 0x74616773 };

	namespace Tag
	{
		constexpr std::uint32_t k_Hlmt{ 0x686C6D74 };
		constexpr std::uint32_t k_Phmo{ 0x70686D6F };
		constexpr std::uint32_t k_Coll{ 0x636F6C6C };
		constexpr std::uint32_t k_Vehi{ 0x76656869 };
		constexpr std::uint32_t k_Mode{ 0x6D6F6465 };
		constexpr std::uint32_t k_Bloc{ 0x626C6F63 };
		constexpr std::uint32_t k_Eqip{ 0x65716970 };
		constexpr std::uint32_t k_Scen{ 0x7363656E };
		constexpr std::uint32_t k_Sbsp{ 0x73627370 };
		constexpr std::uint32_t k_Scnr{ 0x73636E72 };
		constexpr std::uint32_t k_Weap{ 0x77656170 };
		constexpr std::uint32_t k_Proj{ 0x70726F6A };
		constexpr std::uint32_t k_Bipd{ 0x62697064 };
		constexpr std::uint32_t k_Ctrl{ 0x6374726C };
		constexpr std::uint32_t k_Zone{ 0x7A6F6E65 };
		constexpr std::uint32_t k_SLdT{ 0x734C6454 };
		constexpr std::uint32_t k_Lbsp{ 0x4C627370 };
		constexpr std::uint32_t k_Play{ 0x706C6179 };
	}
}