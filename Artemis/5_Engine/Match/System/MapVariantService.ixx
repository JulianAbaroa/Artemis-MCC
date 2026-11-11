export module Engine.Match.System:MapVariant;

import :MatchConfig;

import Common.Log.System;
import Engine.Base.MapVariant.Type;
import Engine.Base.MapVariant.State;
import std;

export namespace Engine::Match::System
{
    // Makes a map variant of a recorded match configuration the active one and writes it into the configuration the engine starts with
    // (Q98, D266).
    // note: The name of a map variant is the name of a recorded configuration, and its map variant is the block of that configuration
    // (MatchConfigService::ReadMapVariant): the file is read when the map variant is set, never at the start. Nothing here touches the
    // game memory: ApplyToStart works on a buffer it is given.
    class MapVariantService
    {
    private:
        using Log = Common::Log::System::Log;

        using MapVariantOutcome = Engine::Base::MapVariant::Type::MapVariantOutcome;
        using MapVariantStore = Engine::Base::MapVariant::State::MapVariantStore;

    public:
        MapVariantService(Log& log, MatchConfigService& matchConfigService, MapVariantStore& mapVariantStore) :
            m_Log(log), m_MatchConfigService(matchConfigService), m_MapVariantStore(mapVariantStore) {}
        ~MapVariantService() = default;

        MapVariantService(const MapVariantService&) = delete;
        auto operator=(const MapVariantService&) -> MapVariantService& = delete;

        // Reads the map variant of a recorded configuration and makes it the active one. The next load writes it.
        // return: False if the configuration cannot be read. The active map variant stays as it was and the result is Failed.
        // note: Reads the file: called by the bench or the bridge, never from the game thread.
        auto SetActive(std::string_view name) -> bool;

        // Removes the active map variant and clears the result: the next load keeps the map variant of its configuration.
        auto ClearActive() -> void;

        // return: Whether a map variant is active.
        auto HasActive() const -> bool;

        // Writes the active map variant into the configuration the engine starts with, over the one it carries, after checking that it fits
        // (same map id, version 0x20, same number of quotas). A map variant that does not fit, or that does not read back equal, leaves the
        // buffer as it was. Writes the result into the store.
        // param configuration: Out. The configuration of the start (the whole match configuration).
        // return: What happened. NoMap when no map variant is active (the result is not touched).
        // note: Called by the start hook on the MCC thread. Never throws.
        auto ApplyToStart(std::span<std::byte> configuration) -> MapVariantOutcome;

    private:
        Log& m_Log;
        MatchConfigService& m_MatchConfigService;
        MapVariantStore& m_MapVariantStore;

        // Writes the result into the store and logs a refusal.
        // return: The outcome.
        auto Finish(MapVariantOutcome outcome, std::string_view name, std::string message) -> MapVariantOutcome;
    };
}