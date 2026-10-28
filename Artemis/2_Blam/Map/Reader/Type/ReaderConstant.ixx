export module Map.Reader.Type:Constant;

import std;

export namespace Map::Reader::Type::Constant
{
    // Footer at the end of the fixup information block. It holds the vertex and index buffer counts.
    constexpr std::int64_t k_FooterSize{ 24 };
    constexpr std::int64_t k_FooterVBCountOffset{ 0 };
    constexpr std::int64_t k_FooterIBCountOffset{ 12 };

    // Buffer info tables. Each entry is k_InfoStride bytes. The vertex buffer entries
    // are followed by k_AuxSize bytes per vertex buffer, then the index buffer entries.
    constexpr std::int32_t k_InfoStride{ 28 };
    constexpr std::int32_t k_AuxSize{ 12 };
    constexpr std::int32_t k_InfoAuxOffset{ 0 };
    constexpr std::int32_t k_InfoDataLengthOffset{ 8 };

    // Masks applied to a fixup address and to a resource datum.
    constexpr std::uint32_t k_FixupMask{ 0x0FFFFFFF };
    constexpr std::uint32_t k_ResourceDatumMask{ 0xFFFFu };

    // Vertex layout. The stride is only a fallback when it cannot be derived from the buffer.
    constexpr std::uint32_t k_VertexBufferStride{ 0x24 };
    constexpr std::int32_t k_XOffset{ 0 };
    constexpr std::int32_t k_YOffset{ 4 };
    constexpr std::int32_t k_ZOffset{ 8 };
    constexpr std::uint16_t k_MeshFlagUnindexed{ (1u << 4) };

    // Instanced geometry entries. The transform fixup is counted from the end of the fixup list.
    constexpr std::int32_t k_InstanceStride{ 156 };
    constexpr int k_InstanceFixupFromEnd{ 10 };
    constexpr int k_InstanceMatrixFloats{ 12 };
    constexpr int k_InstanceScaledFloats{ 9 };
    constexpr int k_InstanceScaleOffset{ 0 };
    constexpr int k_InstanceMatrixOffset{ 4 };
    constexpr int k_InstanceSectionOffset{ 58 };
}