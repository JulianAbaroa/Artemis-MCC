export module Blam.Map.Reader.Type:Constant;

import std;

export namespace Blam::Map::Reader::Type::Constant
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

    // The same entries also hold the index of the instanced geometry definition they place and its flags.
    constexpr int k_InstanceDefinitionOffset{ 52 };
    constexpr int k_InstanceFlagsOffset{ 54 };

    // Location bits of a resource fixup (AddressLocationHighBits): the target is in the definition or in the raw page.
    constexpr std::uint8_t k_FixupLocationMask{ 0x60 };
    constexpr std::uint8_t k_FixupLocationDefinition{ 0x20 };
    constexpr std::uint8_t k_FixupLocationPage{ 0x40 };

    // Tag block inside a resource definition: count, address (patched by a fixup), definition.
    constexpr std::int32_t k_TagBlockSize{ 12 };
    constexpr std::int32_t k_TagBlockAddressOffset{ 4 };

    // Root of the collision resource of a structure bsp: three tag blocks, the bsps with the normal layout, the bsps with the
    // large layout and the instanced geometry definitions.
    constexpr std::int32_t k_CollisionRootSize{ 36 };
    constexpr std::int32_t k_CollisionRootNormalBlock{ 0 };
    constexpr std::int32_t k_CollisionRootLargeBlock{ 12 };
    constexpr std::int32_t k_CollisionRootInstancedBlock{ 24 };

    // A collision geometry is nine tag blocks in a row: nodes, supernodes, planes, leaves, bsp2d references, bsp2d nodes,
    // surfaces, edges and vertices. Offsets of the ones that are read.
    constexpr std::int32_t k_CollisionNodesBlock{ 0 };
    constexpr std::int32_t k_CollisionSurfacesBlock{ 72 };
    constexpr std::int32_t k_CollisionEdgesBlock{ 84 };
    constexpr std::int32_t k_CollisionVerticesBlock{ 96 };

    // Entry of the instanced geometry definitions: the collision geometry (normal layout) and, after it, a second one that
    // would use the large layout.
    constexpr std::int32_t k_InstanceDefinitionStride{ 324 };
    constexpr std::int32_t k_InstanceDefinitionCollisionOffset{ 8 };
    constexpr std::int32_t k_InstanceDefinitionLargeOffset{ 116 };
}