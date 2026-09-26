export module Resolved.World.Type:Constant;

import std;

export namespace Resolved::World::Type::Constant
{
	// bit0: "Compressed Position"
	constexpr std::uint16_t k_CompressedPositionFlag{ 1u << 0 };

    constexpr std::uint16_t k_ShapeSphere{ 0x0 };
    constexpr std::uint16_t k_ShapePill{ 0x1 };
    constexpr std::uint16_t k_ShapeBox{ 0x2 };
    constexpr std::uint16_t k_ShapeTriangle{ 0x3 };
    constexpr std::uint16_t k_ShapePolyhedron{ 0x4 };
    constexpr std::uint16_t k_ShapeMultiSphere{ 0x5 };
    constexpr std::uint16_t k_ShapePhantom{ 0x6 };
    constexpr std::uint16_t k_ShapeList{ 0xE };
    constexpr std::uint16_t k_ShapeMOPP{ 0xF };

    constexpr float k_CellSize{ 16.0f };
}