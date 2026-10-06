export module Platform.Memory.Type:Scanner;

import std;

export namespace Platform::Memory::Type
{
    // Scan mode. Differential modes compare two snapshots, the others filter by value.
    enum class Mode : std::uint8_t
    {
        Changed,        // any byte changed
        Unchanged,      // no byte changed (reverse filter)
        Increased,      // value increased (requires DataType)
        Decreased,      // value decreased (requires DataType)
        IncreasedBy,    // increased exactly by ValueA (requires DataType)
        DecreasedBy,    // decreased exactly by ValueA (requires DataType)

        ExactValue,     // value == ValueA (requires DataType)
        InRange,        // ValueA <= value <= ValueB (requires DataType)
        BitMask,        // (value & BitMaskPattern) == BitMaskValue
    };

    enum class DataType : std::uint8_t
    {
        Bytes,      // byte-by-byte comparison, no interpretation
        Int8,
        UInt8,
        Int16,
        UInt16,
        Int32,
        UInt32,
        Float32,
    };

    // return: Size in bytes of the data type. Bytes counts as 1.
    inline auto ScanDataTypeSize(DataType type) -> std::size_t
    {
        switch (type)
        {
        case DataType::Int8:
        case DataType::UInt8:
            return 1;

        case DataType::Int16:
        case DataType::UInt16:
            return 2;

        case DataType::Int32:
        case DataType::UInt32:
        case DataType::Float32:
            return 4;

        default:
            return 1;
        }
    }

    struct Filter
    {
        Mode Mode{ Mode::Changed };
        DataType DataType{ DataType::Bytes };

        // Reference values stored as raw bits, reinterpreted according to DataType.
        // note: For Float32, use std::bit_cast<std::uint32_t>(float) before assigning.
        std::uint64_t ValueA{ 0 };  // exact value, lower limit or delta
        std::uint64_t ValueB{ 0 };  // upper limit (InRange only)

        std::uint32_t BitMaskPattern{ 0 };  // mask to apply (BitMask only)
        std::uint32_t BitMaskValue{ 0 };    // expected value after the mask (BitMask only)
    };

    struct TypedMatch
    {
        std::size_t Offset{ 0 };
        std::uint64_t ValueBefore{ 0 };  // raw bits
        std::uint64_t ValueAfter{ 0 };   // raw bits
        DataType DataType{ DataType::Bytes };
    };

    struct Region
    {
        std::string Name{};
        std::uintptr_t BaseAddress{ 0 };
        std::size_t Size{ 0 };
    };

    struct ByteDiff
    {
        std::size_t Offset{};
        std::uint8_t Before{};
        std::uint8_t After{};
    };

    struct Snapshot
    {
        std::vector<std::uint8_t> Data{};
        std::string Label{};
        std::uintptr_t BaseAddress{ 0 };
    };

    struct Round
    {
        Snapshot Before{};
        Snapshot After{};
        std::vector<ByteDiff> Diffs{};          // byte modes result
        std::vector<TypedMatch> TypedDiffs{};   // typed modes result
        bool IsComplete{ false };
        bool IsUnchangedRound{ false };
    };

    struct Session
    {
        Region Region{};
        Filter Filter{};
        std::vector<Round> Rounds{};

        // Intersection of the completed rounds.
        std::vector<ByteDiff> FinalDiffs{};         // byte modes
        std::vector<TypedMatch> FinalMatches{};     // typed modes
    };
}