export module Platform.Memory.Type:Scanner;

import std;

export namespace Platform::Memory::Type
{
    enum class Mode : std::uint8_t
    {
        // Differential: compare two snapshots
        Changed,            // any byte changed
        Unchanged,          // no byte changed (reverse filter)
        Increased,          // value increased (requires DataType)
        Decreased,          // value decreased (requires DataType)
        IncreasedBy,        // increased exactly by ValueA (requires DataType)
        DecreasedBy,        // decreased exactly by ValueA (requires DataType)

        // Exact value: searches in a snapshot
        ExactValue,         // value == ValueS (required Data Type)

        // Range: value between Value and Value
        InRange,            // ValueS <= value <= ValueS (required Data Type)

        // Bitmask: offsets where (value & BitMask) == BitPattern
        BitMask,

        // Stabilization: value that stopped changing after N rounds
        Stabilized,
    };

    enum class DataType : std::uint8_t
    {
        Bytes,      // byte-by-byte comparison (no interpretation)
        Int8,
        UInt8,
        Int16,
        UInt16,
        Int32,
        UInt32,
        Float32,
    };

    inline std::size_t ScanDataTypeSize(DataType t)
    {
        switch (t)
        {
        case DataType::Int8:
        case DataType::UInt8:   return 1;
        case DataType::Int16:
        case DataType::UInt16:  return 2;
        case DataType::Int32:
        case DataType::UInt32:
        case DataType::Float32: return 4;
        default:                    return 1;
        }
    }

    struct Filter
    {
        Mode Mode{ Mode::Changed };
        DataType DataType{ DataType::Bytes };

        // Reference value(s) — stored as raw bits, reinterpreted according to DataType.
        // For Float32 : use std::bit_cast<uint32_t>(float) before assigning.
        std::uint64_t ValueA{ 0 };   // exact / lower limit / delta
        std::uint64_t ValueB{ 0 };   // upper limit (InRange only)

        // For BitMask:
        std::uint32_t BitMaskPattern{ 0 }; // bitmask to verify
        std::uint32_t BitMaskValue{ 0 };   // Expected value after applying the mask

        // For Stabilized:
        int StabilizeRounds{ 3 };    // How many consecutive rounds without a change?
    };

    struct TypedMatch
    {
        std::size_t Offset{ 0 };
        std::uint64_t ValueBefore{ 0 };  // raw bits
        std::uint64_t ValueAfter{ 0 };   // raw bits
        DataType DataType{ DataType::Bytes };

        // Reading helpers
        auto AsFloat() const -> float
        { 
            float f; 
            std::memcpy(&f, &ValueAfter, 4); 
            return f; 
        }

        auto AsInt32() const -> std::int32_t
        { 
            return static_cast<std::int32_t>(ValueAfter); 
        }

        auto AsUInt32() const -> std::uint32_t
        { 
            return static_cast<std::uint32_t>(ValueAfter); 
        }
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
        std::vector<ByteDiff> Diffs{};        // result byte by byte (raw modes)
        std::vector<TypedMatch> TypedDiffs{}; // typed result (typed modes)
        bool IsComplete{ false };
        bool IsUnchangedRound{ false };
    };

    struct Session
    {
        Region Region{};
        Filter Filter{};
        std::vector<Round> Rounds{};

        // Final results
        std::vector<ByteDiff> FinalDiffs{};       // for Byte modes
        std::vector<TypedMatch> FinalMatches{};   // for typed modes
    };
}