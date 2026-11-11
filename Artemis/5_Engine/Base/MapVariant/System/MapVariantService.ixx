export module Engine.Base.MapVariant.System:MapVariant;

import Common.Log.System;
import Engine.Base.MapVariant.Type;
import Engine.Base.MapVariant.State;
import std;

export namespace Engine::Base::MapVariant::System
{
    // Makes a map variant the active one and writes it into the configuration the engine starts with, or into the live map variant at a
    // round break (Q98, D266).
    // note: The block of a map variant is read from a recorded configuration by the caller (MatchConfigService::ReadMapVariant) when the
    // request arrives, so the game thread never reads a disk. Nothing here touches the game memory: the writing methods work on buffers
    // they are given.
    class MapVariantService
    {
    private:
        using Log = Common::Log::System::Log;

        using MapVariantOutcome = Engine::Base::MapVariant::Type::MapVariantOutcome;
        using MapVariantStore = Engine::Base::MapVariant::State::MapVariantStore;

    public:
        MapVariantService(Log& log, MapVariantStore& mapVariantStore) : m_Log(log), m_MapVariantStore(mapVariantStore) {}
        ~MapVariantService() = default;

        MapVariantService(const MapVariantService&) = delete;
        auto operator=(const MapVariantService&) -> MapVariantService& = delete;

        // Makes the map variant of a recorded configuration the active one. The next load writes it. Drops the request waiting.
        // param block: The map variant read from the configuration. Empty when it could not be read.
        // return: False if the block is empty. The active map variant stays as it was and the result is Failed.
        auto SetActive(std::string_view name, std::vector<std::byte> block) -> bool;

        // Removes the active map variant and clears the result: the next load keeps the map variant of its configuration.
        auto ClearActive() -> void;

        // Asks for a map variant by name, or for none with an empty name (the one the match loaded with). In the lobby it is written at the
        // next start, in a match at the next round break.
        // param block: The map variant read from the configuration (ignored for none). Empty when it could not be read.
        // return: False if the block is empty for a name. The request waiting from before and the active map variant stay as they were and
        // the result is Failed.
        // note: Called by the bridge thread.
        auto Request(std::string_view name, std::vector<std::byte> block, bool isInMatch) -> bool;

        // Drops the request that still waits for a round break, when the client that made it is gone.
        auto CancelPending() -> void;

        // return: Whether a map variant is active.
        auto HasActive() const -> bool;

        // return: Whether a request waits for a round break. Lock free.
        auto HasPending() const -> bool;

        // Starts the bookkeeping of an engine start: keeps the map variant the configuration carries as the one a request for none brings
        // back, and takes the request no round break served (the match ended first) as the wanted map variant of this start.
        // param configuration: The configuration the engine starts with, before the active map variant is written into it.
        // note: Called by the start hook on the MCC thread, on every start. Never throws.
        auto BeginStart(std::span<const std::byte> configuration) -> void;

        // Writes the active map variant into the configuration the engine starts with, over the one it carries, after checking that it fits
        // (same map id, version 0x20, same number of quotas). A map variant that does not fit, or that does not read back equal, leaves the
        // buffer as it was. Writes the result into the store.
        // param configuration: Out. The configuration of the start (the whole match configuration).
        // return: What happened. NoMap when no map variant is active (the result is not touched).
        // note: Called by the start hook on the MCC thread. Never throws.
        auto ApplyToStart(std::span<std::byte> configuration) -> MapVariantOutcome;

        // Serves the request waiting into the live map variant, at a round break only. It waits (nothing is written, the request stays)
        // unless the round has ended or is between rounds and the game mode is multiplayer. The request is compared with the runtime map
        // variant (same map id, version, quotas): if it does not fit, runtime and options stay byte for byte as they were (Rejected). If it
        // fits, the runtime map variant gets the new block with its own world bounds and the options get the block, both are read back,
        // and a mismatch restores both (Failed). One application per request.
        // param runtime: Out. The map variant the engine builds the round from (k_MapVariantSize bytes).
        // param options: Out. The copy of it in the game options (k_MapVariantSize bytes).
        // note: Game thread, inside the tick hook. Never throws.
        auto ApplyBetweenRounds(std::span<std::byte> runtime, std::span<std::byte> options, std::uint8_t roundState,
            std::uint8_t gameMode) -> void;

    private:
        Log& m_Log;
        MapVariantStore& m_MapVariantStore;

        // Writes the result into the store (of the request when there is one) and logs a refusal.
        // return: The outcome.
        auto Finish(std::optional<std::uint64_t> requestId, MapVariantOutcome outcome, std::string_view name, std::string message)
            -> MapVariantOutcome;
    };
}