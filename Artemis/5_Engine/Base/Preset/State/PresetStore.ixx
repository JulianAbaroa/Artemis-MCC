export module Engine.Base.Preset.State:Preset;

import Engine.Base.Preset.Type;
import Blam.Layout.Game.Type;
import std;

export namespace Engine::Base::Preset::State
{
    // The active gametype preset, the catalog it was loaded with, the base values saved for the match with the catalog they were saved
    // with, the request waiting for a round break and the result of the last request or load (D264, D265).
    // note: Thread-safe, and data only. Written by the bench, the bridge and the start hook (MCC threads) and the tick hook (game
    // thread), read by the hooks and the Workbench. The snapshots it hands out are immutable, so a reader keeps a consistent preset while
    // another one is set.
    class PresetStore
    {
    private:
        using CompiledPreset = Blam::Layout::Game::Type::VariantPreset::CompiledPreset;
        using PresetBase = Blam::Layout::Game::Type::VariantPreset::PresetBase;
        using PresetCatalog = Blam::Layout::Game::Type::VariantPreset::PresetCatalog;

        using PresetResult = Engine::Base::Preset::Type::PresetResult;

    public:
        // A preset requested during a match that waits for a round break. A null preset asks for none: the base values come back.
        // Id is the one BeginRequest gave: the result of a request is only written while it is the latest one.
        struct PendingRequest
        {
            std::uint64_t Id{ 0 };
            std::string Name{};
            std::shared_ptr<const CompiledPreset> Preset{};
            std::shared_ptr<const PresetCatalog> Catalog{};
        };

        PresetStore() = default;
        ~PresetStore() = default;

        PresetStore(const PresetStore&) = delete;
        auto operator=(const PresetStore&) -> PresetStore& = delete;

        // return: The active preset, or null when there is none.
        auto GetActive() const -> std::shared_ptr<const CompiledPreset>;

        // return: The name of the active preset, empty when there is none.
        auto GetActiveName() const -> std::string;

        // Makes a preset the active one, or none with null. The next load writes it.
        auto SetActive(std::shared_ptr<const CompiledPreset> preset) -> void;

        // return: The catalog the active preset was loaded with, or null.
        auto GetCatalog() const -> std::shared_ptr<const PresetCatalog>;

        auto SetCatalog(std::shared_ptr<const PresetCatalog> catalog) -> void;

        // return: The value every field of the catalog had before the preset was written in this match, or null.
        auto GetBase() const -> std::shared_ptr<const PresetBase>;

        // return: The catalog the base was saved with, or null when there is no base. Anything composed over the base uses it.
        auto GetBaseCatalog() const -> std::shared_ptr<const PresetCatalog>;

        // Keeps the base with the catalog it was saved with. Null drops both.
        auto SetBase(std::shared_ptr<const PresetBase> base, std::shared_ptr<const PresetCatalog> catalog = nullptr) -> void;

        // return: A new request id. It is the latest one until the next call.
        auto BeginRequest() -> std::uint64_t;

        // return: Whether a request waits for a round break. Lock free: the game thread asks it every frame.
        auto HasPending() const -> bool;

        // Leaves a request to serve at a round break, replacing the one waiting.
        auto SetPending(PendingRequest request) -> void;

        // Reads the request waiting and clears it in one step.
        // return: The request, or none.
        auto TakePending() -> std::optional<PendingRequest>;

        // Drops the request waiting.
        auto ClearPending() -> void;

        auto GetResult() const -> PresetResult;
        auto SetResult(PresetResult result) -> void;

        // Sets the result only if the request is still the latest one, so the end of an old request never hides a newer one.
        // return: Whether it was set.
        auto SetResultOf(std::uint64_t requestId, PresetResult result) -> bool;

    private:
        mutable std::mutex m_Mutex{};
        std::shared_ptr<const CompiledPreset> m_Active{};
        std::shared_ptr<const PresetCatalog> m_Catalog{};
        std::shared_ptr<const PresetBase> m_Base{};
        std::shared_ptr<const PresetCatalog> m_BaseCatalog{};
        std::optional<PendingRequest> m_Pending{};
        std::uint64_t m_LatestRequestId{ 0 };
        PresetResult m_Result{};

        std::atomic<bool> m_HasPending{ false };
    };
}