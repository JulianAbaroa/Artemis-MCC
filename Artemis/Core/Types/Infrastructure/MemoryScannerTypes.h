#pragma once

#include <string>
#include <vector>
#include <cstdint>

enum class ScanMode : uint8_t
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

enum class ScanDataType : uint8_t
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

inline size_t ScanDataTypeSize(ScanDataType t)
{
    switch (t)
    {
    case ScanDataType::Int8:
    case ScanDataType::UInt8:   return 1;
    case ScanDataType::Int16:
    case ScanDataType::UInt16:  return 2;
    case ScanDataType::Int32:
    case ScanDataType::UInt32:
    case ScanDataType::Float32: return 4;
    default:                    return 1;
    }
}

struct ScanFilter
{
    ScanMode Mode = ScanMode::Changed;
    ScanDataType DataType = ScanDataType::Bytes;

    // Reference value(s) — stored as raw bits, reinterpreted according to DataType.
    // For Float32 : use std::bit_cast<uint32_t>(float) before assigning.
    uint64_t ValueA = 0;   // exact / lower limit / delta
    uint64_t ValueB = 0;   // upper limit (InRange only)

    // For BitMask:
    uint32_t BitMaskPattern = 0; // bitmask to verify
    uint32_t BitMaskValue = 0;   // Expected value after applying the mask

    // For Stabilized:
    int StabilizeRounds = 3;    // How many consecutive rounds without a change?
};

struct TypedMatch
{
    size_t Offset = 0;
    uint64_t ValueBefore = 0;  // raw bits
    uint64_t ValueAfter = 0;   // raw bits
    ScanDataType DataType = ScanDataType::Bytes;

    // Reading helpers
    float AsFloat() const { float f; std::memcpy(&f, &ValueAfter, 4); return f; }
    int32_t AsInt32() const { return static_cast<int32_t>(ValueAfter); }
    uint32_t AsUInt32() const { return static_cast<uint32_t>(ValueAfter); }
};

struct ScanRegion
{
    std::string Name;
    uintptr_t BaseAddress = 0;
    size_t Size = 0;
};

struct ByteDiff
{
    size_t  Offset;
    uint8_t Before;
    uint8_t After;
};

struct ScanSnapshot
{
    std::vector<uint8_t> Data;
    std::string Label;
    uintptr_t BaseAddress = 0;
};

struct ScanRound
{
    ScanSnapshot Before;
    ScanSnapshot After;
    std::vector<ByteDiff> Diffs;        // result byte by byte (raw modes)
    std::vector<TypedMatch> TypedDiffs; // typed result (typed modes)
    bool IsComplete = false;
    bool IsUnchangedRound = false;
};

struct ScanSession
{
    ScanRegion Region;
    ScanFilter Filter;
    std::vector<ScanRound> Rounds;

    // Final results
    std::vector<ByteDiff> FinalDiffs;       // for Byte modes
    std::vector<TypedMatch> FinalMatches;   // for typed modes
};