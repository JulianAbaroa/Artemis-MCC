export module Platform.Memory.State:Scanner;

import Platform.Memory.Type;
import std;

export namespace Platform::Memory::State
{
    // State of the memory scanner: scan session and scanning flag.
    // note: Written by the scan worker thread and read by the UI without locks.
    class MemoryScannerStore
    {
    private:
        using Session = Platform::Memory::Type::Session;
        using Filter = Platform::Memory::Type::Filter;
        using Snapshot = Platform::Memory::Type::Snapshot;
        using ByteDiff = Platform::Memory::Type::ByteDiff;
        using TypedMatch = Platform::Memory::Type::TypedMatch;
        using Round = Platform::Memory::Type::Round;

    public:
        MemoryScannerStore() = default;
        ~MemoryScannerStore() = default;

        auto GetSession() const -> const Session&;
        auto GetFilter() const -> const Filter&;
        auto IsScanning() const -> bool;

        auto SetFilter(const Filter& filter) -> void;
        auto SetRegion(const std::string& name, std::uintptr_t base, std::size_t size) -> void;
        auto SetScanning(bool isScanning) -> void;

        // Opens a round. The Set/Complete methods below act on the last one and do nothing without rounds.
        auto BeginRound() -> void;
        auto BeginUnchangedRound() -> void;
        auto SetRoundBefore(Snapshot snapshot) -> void;
        auto SetRoundAfter(Snapshot snapshot) -> void;
        auto SetRoundDiffs(std::vector<ByteDiff> diffs) -> void;
        auto SetRoundTypedDiffs(std::vector<TypedMatch> matches) -> void;
        auto CompleteRound() -> void;

        // Intersects the completed rounds into the final result, byte or typed according to the filter.
        // note: Unchanged rounds remove from the result the offsets that changed (byte modes only).
        auto ComputeFinalDiffs() -> void;

        // Clears the session and the scanning flag.
        auto Reset() -> void;

    private:
        Session m_Session{};
        bool m_IsScanning{ false };
    };
}