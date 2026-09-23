export module Platform.Memory.State:Scanner;

import Platform.Memory.Type;
import std;

export namespace Platform::Memory::State
{
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

        const Session& GetSession() const;

        auto GetDelayMs() const -> int;
        auto SetDelayMs(int ms) -> void;

        auto GetFilter() const -> const Filter&;
        auto SetFilter(const Filter& filter) -> void;

        auto SetRegion(const std::string& name, std::uintptr_t base, std::size_t size) -> void;
        auto SetWaitingForKey(bool waiting) -> void;

        auto BeginRound() -> void;
        auto BeginUnchangedRound() -> void;
        auto SetRoundBefore(Snapshot snap) -> void;
        auto SetRoundAfter(Snapshot snap) -> void;
        auto SetRoundDiffs(std::vector<ByteDiff> diffs) -> void;
        auto SetRoundTypedDiffs(std::vector<TypedMatch> matches) -> void;
        auto CompleteRound() -> void;
        auto ComputeFinalDiffs() -> void;
        auto Reset() -> void;

        auto IsWaitingForKey() const -> bool;
        auto IsScanning() const -> bool;
        auto SetScanning(bool scanning) -> void;

    private:
        Session m_Session{};
        int m_DelayMs{ 500 };
        bool m_WaitingForKey{ false };
        bool m_Scanning{ false };
    };
}