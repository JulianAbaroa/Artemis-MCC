export module Map.Reader.Type:Constant;

import std;

export namespace Map::Reader::Type::Constant
{
	constexpr std::int64_t k_FooterSize{ 24 };
	constexpr std::int64_t k_FooterVBCountOffset{ 0 };
	constexpr std::int64_t k_FooterIBCountOffset{ 12 };

	constexpr std::int32_t k_InfoStride{ 28 };
	constexpr std::int32_t k_AuxSize{ 12 };
	constexpr std::int32_t k_InfoAuxOffset{ 0 };
	constexpr std::int32_t k_InfoDataLengthOffset{ 8 };

	constexpr std::uint32_t k_FixupMask{ 0x0FFFFFFF };
	constexpr std::uint32_t k_ResourceDatumMask{ 0xFFFFu };

	constexpr std::uint32_t k_VertexBufferStride{ 0x24 };
	constexpr std::int32_t k_XOffset{ 0 };
	constexpr std::int32_t k_YOffset{ 4 };
	constexpr std::int32_t k_ZOffset{ 8 };
	constexpr std::uint16_t k_MeshFlagUnindexed{ (1u << 4) };

	constexpr std::int32_t k_InstanceStride{ 156 };
	constexpr int k_InstanceFixupFromEnd{ 10 };
	constexpr int k_InstanceMatrixFloats{ 12 };
	constexpr int k_InstanceScaledFloats{ 9 };
	constexpr int k_InstanceScaleOffset{ 0 };
	constexpr int k_InstanceMatrixOffset{ 4 };
	constexpr int k_InstanceSectionOffset{ 58 };
}