export module Bridge.Codec.Flat.State;

import Bridge.Codec.Flat.Type;
import std;

export namespace Bridge::Codec::Flat::State
{
    // The Tick of the last decision tick, handed from the AI thread to the bridge thread.
    // note: One slot. A request the bridge thread has not taken yet is replaced by the next one (the client only needs the newest
    // state, and in Lockstep the game waits, so it never happens there).
    class FlatStore
    {
    private:
        using FlatRequest = Bridge::Codec::Flat::Type::FlatRequest;
        using milliseconds = std::chrono::milliseconds;

    public:
        FlatStore() = default;
        ~FlatStore() = default;

        FlatStore(const FlatStore&) = delete;
        auto operator=(const FlatStore&) -> FlatStore& = delete;

        auto Push(FlatRequest request) -> void;

        // return: The pending request, or nothing if none arrived within the timeout.
        auto WaitPop(milliseconds timeout) -> std::optional<FlatRequest>;

        auto Cleanup() -> void;

    private:
        std::optional<FlatRequest> m_Pending{};
        std::mutex m_Mutex{};
        std::condition_variable m_Condition{};
    };
}