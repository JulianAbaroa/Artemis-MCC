#pragma once

#include <cstdint>

namespace MapMagics
{
	// 'tags' magic.
	static constexpr uint32_t k_IndexHeaderMagic = 0x74616773;

	static constexpr uint32_t k_HlmtMagic = 0x686C6D74;
	static constexpr uint32_t k_PhmoMagic = 0x70686D6F;
	static constexpr uint32_t k_CollMagic = 0x636F6C6C;
	static constexpr uint32_t k_VehiMagic = 0x76656869;
	static constexpr uint32_t k_ModeMagic = 0x6D6F6465;
	static constexpr uint32_t k_BlocMagic = 0x626C6F63;
	static constexpr uint32_t k_EqipMagic = 0x65716970;
	static constexpr uint32_t k_ScenMagic = 0x7363656E;
	static constexpr uint32_t k_SbspMagic = 0x73627370;
	static constexpr uint32_t k_ScnrMagic = 0x73636E72;
	static constexpr uint32_t k_WeapMagic = 0x77656170;
	static constexpr uint32_t k_ProjMagic = 0x70726F6A;
	static constexpr uint32_t k_BipdMagic = 0x62697064;
	static constexpr uint32_t k_JmadMagic = 0x6A6D6164;
	static constexpr uint32_t k_CtrlMagic = 0x6374726C;
}