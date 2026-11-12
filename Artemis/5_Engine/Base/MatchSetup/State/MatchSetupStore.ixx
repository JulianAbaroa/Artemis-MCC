export module Engine.Base.MatchSetup.State:MatchSetup;

import Engine.Base.MatchSetup.Type;
import std;

export namespace Engine::Base::MatchSetup::State
{
    // The match setup waiting to be applied, the result of the last request, the bots the engine holds and what the start hands to the
    // first tick (D286).
    // note: Thread-safe, and data only. Requests come from the bench, the bridge and the Workbench (their threads), the start hook runs on
    // the MCC thread and the tick hooks on the game thread. A request replaces the one waiting, and the result of an older request never
    // replaces the one of a newer request. The preset and the map variant the engine holds live in their own stores.
    class MatchSetupStore
    {
    private:
        using MatchSetup = Engine::Base::MatchSetup::Type::MatchSetup;
        using MatchSetupLoad = Engine::Base::MatchSetup::Type::MatchSetupLoad;
        using MatchSetupOrigin = Engine::Base::MatchSetup::Type::MatchSetupOrigin;
        using MatchSetupRequest = Engine::Base::MatchSetup::Type::MatchSetupRequest;
        using MatchSetupResult = Engine::Base::MatchSetup::Type::MatchSetupResult;
        using Roster = Engine::Base::MatchSetup::Type::Roster;

    public:
        MatchSetupStore() = default;
        ~MatchSetupStore() = default;

        MatchSetupStore(const MatchSetupStore&) = delete;
        auto operator=(const MatchSetupStore&) -> MatchSetupStore& = delete;

        // Leaves a setup to apply at the next start or the next round break, replacing the one waiting, and sets the result to Pending.
        // param isMerge: Takes the fields the setup has over the one waiting when that one has the same origin (the bridge asks for the
        // preset and the map in two calls and wants both), so the setup that waits is the old one plus the new fields.
        // return: The number of the request.
        auto Request(MatchSetup setup, MatchSetupOrigin origin, bool isMerge = false) -> std::uint64_t;

        // Records a request that cannot be applied: the result is Rejected with the reason and the number of the new request. A request
        // that waited before stays and is still applied, with its own result dropped as an older one.
        // return: The number of the request.
        auto Reject(std::string reason) -> std::uint64_t;

        // return: Whether a request waits. Lock free: the game thread asks it every tick.
        auto HasPending() const -> bool;

        // Reads the request waiting and clears it in one step. The result stays Pending until Finish.
        // return: The request, or none.
        auto TakePending() -> std::optional<MatchSetupRequest>;

        // Reads and clears the request waiting only if canServe says the moment is right for what it holds.
        // note: canServe runs under the lock of the store: it must not call the store.
        auto TakePendingIf(const std::function<bool(const MatchSetup&)>& canServe) -> std::optional<MatchSetupRequest>;

        // Drops the request waiting if the given origin made it, when that client is gone. Its Pending result goes back to neutral.
        auto DropPending(MatchSetupOrigin origin) -> void;

        // return: The number of the latest request.
        auto GetSequence() const -> std::uint64_t;

        auto GetResult() const -> MatchSetupResult;

        // Records what happened to a request, in one step under the lock.
        // param adoptedBots: The bots the engine holds now, when the request set them: they become the active roster, which the first tick
        // of every match applies. It is kept even when the result of an older request is not.
        // note: The result is only replaced if it does not belong to an older request than the one shown, so an old apply never hides a
        // newer Pending or Rejected.
        auto Finish(const MatchSetupResult& result, const std::optional<Roster>& adoptedBots) -> void;

        // return: The bots the engine holds, which the start of every match applies again. Empty when none were asked for.
        auto GetActiveBots() const -> std::optional<Roster>;

        // Leaves the bots of the start for the first tick.
        auto SetLoad(MatchSetupLoad load) -> void;

        // return: What the start left for the first tick, and clears it.
        auto TakeLoad() -> std::optional<MatchSetupLoad>;

        // Whether the first tick of the match went by. Cleanup resets it.
        auto IsMatchStarted() const -> bool;
        auto SetMatchStarted() -> void;

        // Whether a request was waiting at the first tick: it is served in the first frame after it, as part of the load (AtLoad).
        auto IsLoadWindow() const -> bool;
        auto SetLoadWindow(bool value) -> void;

        // Forgets what belongs to the match that ended. The active bots and the request waiting stay, like the preset and the map variant:
        // the next match applies them. The result goes back to neutral (Pending if a request waits), so the lobby does not show what
        // happened in the match.
        auto Cleanup() -> void;

    private:
        mutable std::mutex m_Mutex{};
        std::optional<MatchSetupRequest> m_Pending{};
        std::uint64_t m_Sequence{ 0 };
        MatchSetupResult m_Result{};
        std::optional<Roster> m_ActiveBots{};
        std::optional<MatchSetupLoad> m_Load{};
        bool m_IsMatchStarted{ false };
        bool m_IsLoadWindow{ false };

        std::atomic<bool> m_HasPending{ false };
    };
}