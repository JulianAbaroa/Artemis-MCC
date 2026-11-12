export module Engine.Base.MapVariant.System:MapVariant;

import Common.Log.System;
import Engine.Base.MapVariant.Type;
import Engine.Base.MapVariant.State;
import std;

export namespace Engine::Base::MapVariant::System
{
    // Reads a map variant block and writes it into the configuration the engine starts with, or into the live map variant at a round
    // break (Q98, D266). When and in which order it happens is decided by MatchSetupService (D286), which also keeps the requests and the
    // results.
    // note: The block of a map variant is read from a recorded configuration by the caller (MatchConfigService::ReadMapVariant) when the
    // request arrives, so the game thread never reads a disk. Nothing here touches the game memory: the writing methods work on buffers
    // they are given.
    class MapVariantService
    {
    private:
        using Log = Common::Log::System::Log;

        using ActiveMapVariant = Engine::Base::MapVariant::Type::ActiveMapVariant;
        using MapVariantLivePlan = Engine::Base::MapVariant::Type::MapVariantLivePlan;
        using MapVariantStore = Engine::Base::MapVariant::State::MapVariantStore;

    public:
        MapVariantService(Log& log, MapVariantStore& mapVariantStore) : m_Log(log), m_MapVariantStore(mapVariantStore) {}
        ~MapVariantService() = default;

        MapVariantService(const MapVariantService&) = delete;
        auto operator=(const MapVariantService&) -> MapVariantService& = delete;

        // Makes a map variant out of the block of a recorded configuration. Nothing in the store changes.
        // param name: Empty for the map variant the match loaded with: the result is null.
        // param block: The map variant read from the configuration. Empty when it could not be read.
        // return: The map variant, or why the block cannot be used (also logged as an ERROR).
        auto Resolve(std::string_view name, std::vector<std::byte> block) const
            -> std::expected<std::shared_ptr<const ActiveMapVariant>, std::string>;

        // return: The active map variant, or null when there is none.
        auto GetActive() const -> std::shared_ptr<const ActiveMapVariant>;

        // return: The name of the active map variant, empty when there is none.
        auto GetActiveName() const -> std::string;

        // Makes the map variant the active one, which every start writes. Null clears the active one.
        auto Adopt(std::shared_ptr<const ActiveMapVariant> mapVariant) -> void;

        // return: Why the map variant can never be written into the match that is running, or nothing: it has to be compatible with the one
        // the start carried (same map id, version, quotas). Nothing is checked before a start. Without memory access, so the thread that asks
        // can check it.
        auto Check(const ActiveMapVariant& mapVariant) const -> std::optional<std::string>;

        // Keeps the map variant the configuration carries as the one a request for none brings back.
        // param configuration: The configuration the engine starts with, before the active map variant is written into it.
        // note: Called by the start hook on the MCC thread, on every start. Never throws.
        auto BeginStart(std::span<const std::byte> configuration) -> void;

        // Writes the map variant into the configuration the engine starts with, over the one it carries, after checking that it fits (same
        // map id, version 0x20, same number of quotas), and reads it back.
        // param configuration: Out. The configuration of the start (the whole match configuration).
        // return: Nothing, or why it was refused or did not read back equal. A failure leaves the buffer as it was.
        // note: Called by the start hook on the MCC thread. Never throws.
        auto WriteToStart(std::span<std::byte> configuration, const ActiveMapVariant& mapVariant) const -> std::expected<void, std::string>;

        // Compares the map variant with the runtime one of a match without writing anything.
        // param mapVariant: Null for the map variant the match loaded with.
        // param runtime: The map variant the engine builds the round from (k_MapVariantSize bytes).
        // return: The plan, or the reason it is refused: the one of the start is not known, or it does not fit (map id, version, quotas).
        // note: Game thread. Never throws.
        auto PrepareLive(std::shared_ptr<const ActiveMapVariant> mapVariant, std::span<const std::byte> runtime) const
            -> std::expected<MapVariantLivePlan, std::string>;

        // Writes the plan into the runtime map variant, with its own world bounds, and its copy into the options, and reads both back. A
        // mismatch restores both.
        // param runtime: Out. The map variant the engine builds the round from (k_MapVariantSize bytes).
        // param options: Out. The copy of it in the game options (k_MapVariantSize bytes).
        // return: Nothing, or why it failed (runtime and options are then as they were).
        // note: Game thread. Never throws. The store does not change: Adopt does it once everything else of the setup was written.
        auto CommitLive(const MapVariantLivePlan& plan, std::span<std::byte> runtime, std::span<std::byte> options) const
            -> std::expected<void, std::string>;

    private:
        Log& m_Log;
        MapVariantStore& m_MapVariantStore;
    };
}