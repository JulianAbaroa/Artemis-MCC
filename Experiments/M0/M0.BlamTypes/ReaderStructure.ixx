export module Map.Reader.Type:Structure;

import std;

// Structures read straight from the map file (reduced copy for the M0 experiment).
// note: They are packed, so their layout must not change.
export namespace Map::Reader::Type::Structure
{
#pragma pack(push, 1)
    namespace Tag
    {
        // Reference to another tag. Size: 0x10.
        struct TagReference
        {
            std::int32_t TagGroupMagic{};
            std::byte _pad_0x04[8]{};
            std::uint32_t DatumIndex{};
        };

        // Header of a block of entries. Size: 0xC.
        struct TagBlock
        {
            std::int32_t EntryCount{};
            std::uint32_t Pointer{};
            std::byte _pad_0x08[4]{};
        };

        struct TagInteropTableEntry
        {
            std::uint32_t Pointer{};
            std::int32_t Type{};
        };

        // Id of a string of the map.
        struct StringId
        {
            std::uint32_t Value{};
        };
    }

    namespace Data
    {
        // Reference to raw data. Size: 0x14.
        struct Reference
        {
            std::int32_t Size{};
            std::byte _pad_0x04[8]{};
            std::uint32_t Pointer{};
            std::byte _pad_0x10[4]{};
        };
    }

    namespace Primitive
    {
        struct Vec2
        {
            float X{};
            float Y{};
        };

        struct Vec3
        {
            float X{};
            float Y{};
            float Z{};
        };

        struct Vec4
        {
            float X{};
            float Y{};
            float Z{};
            float W{};
        };

        struct ColorRGB
        {
            float Red{};
            float Green{};
            float Blue{};
        };

        struct ColorRGBA
        {
            float Red{};
            float Green{};
            float Blue{};
            float Alpha{};
        };

        struct RangeF
        {
            float Min{};
            float Max{};
        };

        struct RangeD
        {
            double Min{};
            double Max{};
        };
    }
#pragma pack(pop)
}