module Engine.Base.MapVariant.System;
import :MapVariant;

import Engine.Base.MapVariant.Type;
import Blam.Layout.Game.Type;
import Blam.Layout.Object.Type;
import std;

namespace
{
    namespace LayoutMapVariant = Blam::Layout::Object::Type::MapVariant;

    using Engine::Base::MapVariant::State::MapVariantStore;
    using Engine::Base::MapVariant::Type::ActiveMapVariant;
    using Engine::Base::MapVariant::Type::MapVariantResult;

    using Blam::Layout::Game::Type::Offset::k_RoundStateBetween;
    using Blam::Layout::Game::Type::Offset::k_RoundStateEnded;
    using Blam::Layout::Object::Type::Constant::k_ConfigurationMapVariantOffset;
    using Blam::Layout::Object::Type::Constant::k_GameModeMultiplayer;
    using Blam::Layout::Object::Type::Constant::k_MapVariantSize;
}

namespace Engine::Base::MapVariant::System
{
    auto MapVariantService::SetActive(std::string_view name, std::vector<std::byte> block) -> bool
    {
        try
        {
            if (block.empty())
            {
                this->Finish(std::nullopt, MapVariantOutcome::Failed, name, "the map variant of the configuration cannot be read");
                return false;
            }

            const auto header{ LayoutMapVariant::Read(block) };
            if (!header)
            {
                this->Finish(std::nullopt, MapVariantOutcome::Failed, name, "the map variant has no header");
                return false;
            }

            // A new id: an application already taken by the game thread can no longer write its result or its choice.
            m_MapVariantStore.BeginRequest();
            m_Log.Message("[MapVariantService] INFO: Map variant \"{}\" (map id {}, {} quotas, {} placements) is active for the next start.",
                name, header->MapId, header->Quotas, header->PlacementsInUse);

            m_MapVariantStore.ClearPending();
            m_MapVariantStore.SetActive(std::make_shared<const ActiveMapVariant>(ActiveMapVariant{ .Name = std::string{ name },
                .Block = std::move(block) }));
            m_MapVariantStore.SetResult(MapVariantResult{});
            return true;
        }
        catch (const std::exception& exception)
        {
            this->Finish(std::nullopt, MapVariantOutcome::Failed, name, exception.what());
            return false;
        }
    }

    auto MapVariantService::ClearActive() -> void
    {
        // A new id: an application already taken by the game thread can no longer write its result or its choice.
        m_MapVariantStore.BeginRequest();
        m_MapVariantStore.ClearPending();
        m_MapVariantStore.SetActive(nullptr);
        m_MapVariantStore.SetResult(MapVariantResult{});
    }

    auto MapVariantService::Request(std::string_view name, std::vector<std::byte> block, bool isInMatch) -> bool
    {
        const std::uint64_t id{ m_MapVariantStore.BeginRequest() };

        try
        {
            std::shared_ptr<const ActiveMapVariant> mapVariant{};

            // A request whose block cannot be used leaves the active map variant as it was. In a match it also drops the request waiting
            // from before, so the map does not change later without anyone being told.
            if (!name.empty())
            {
                const auto header{ LayoutMapVariant::Read(block) };

                if (!header)
                {
                    if (isInMatch) m_MapVariantStore.ClearPending();
                    this->Finish(id, MapVariantOutcome::Failed, name, "the map variant of the configuration cannot be read");
                    return false;
                }

                mapVariant = std::make_shared<const ActiveMapVariant>(ActiveMapVariant{ .Name = std::string{ name }, .Block = std::move(block) });
            }

            if (!isInMatch)
            {
                m_MapVariantStore.ClearPending();
                m_MapVariantStore.SetActive(std::move(mapVariant));
                m_MapVariantStore.SetResultOf(id, MapVariantResult{ .Outcome = MapVariantOutcome::AppliedAtLoad, .Name = std::string{ name } });
                m_Log.Message("[MapVariantService] INFO: Map variant \"{}\" will be written at the next start.", name);
                return true;
            }

            m_MapVariantStore.SetPending(MapVariantStore::PendingRequest{ .Id = id, .Name = std::string{ name },
                .MapVariant = std::move(mapVariant) });
            m_MapVariantStore.SetResultOf(id, MapVariantResult{ .Outcome = MapVariantOutcome::Pending, .Name = std::string{ name } });
            m_Log.Message("[MapVariantService] INFO: Map variant \"{}\" requested, it waits for a round break.", name);
            return true;
        }
        catch (const std::exception& exception)
        {
            this->Finish(id, MapVariantOutcome::Failed, name, exception.what());
            return false;
        }
    }

    auto MapVariantService::CancelPending() -> void
    {
        const auto request{ m_MapVariantStore.TakePending() };
        if (!request) return;

        m_MapVariantStore.SetResultOf(request->Id, MapVariantResult{});
        m_Log.Message("[MapVariantService] INFO: Map variant \"{}\" request dropped.", request->Name);
    }

    auto MapVariantService::HasActive() const -> bool
    {
        return m_MapVariantStore.GetActive() != nullptr;
    }

    auto MapVariantService::HasPending() const -> bool
    {
        return m_MapVariantStore.HasPending();
    }

    auto MapVariantService::BeginStart(std::span<const std::byte> configuration) -> void
    {
        try
        {
            // The map variant the start carries belongs to this match: a request for none brings it back.
            if (configuration.size() >= k_ConfigurationMapVariantOffset + k_MapVariantSize)
            {
                const auto first{ configuration.begin() + k_ConfigurationMapVariantOffset };
                m_MapVariantStore.SetBase(std::make_shared<const std::vector<std::byte>>(first, first + k_MapVariantSize));
            }
            else
            {
                m_MapVariantStore.SetBase(nullptr);
            }

            // A request no round break served (the match ended first) is the wanted map variant of this start, or none.
            const auto request{ m_MapVariantStore.TakePending() };
            if (!request) return;

            m_MapVariantStore.SetActive(request->MapVariant);
            if (request->MapVariant == nullptr)
            {
                m_MapVariantStore.SetResultOf(request->Id, MapVariantResult{ .Outcome = MapVariantOutcome::AppliedAtLoad });
            }

            m_Log.Message("[MapVariantService] INFO: Map variant \"{}\" requested in the last match is taken by this start.", request->Name);
        }
        catch (const std::exception& exception)
        {
            m_Log.Message("[MapVariantService] ERROR: Failed to begin the start ({}).", exception.what());
        }
    }

    auto MapVariantService::ApplyToStart(std::span<std::byte> configuration) -> MapVariantOutcome
    {
        const auto active{ m_MapVariantStore.GetActive() };
        if (active == nullptr) return MapVariantOutcome::NoMap;

        try
        {
            if (configuration.size() < k_ConfigurationMapVariantOffset + k_MapVariantSize)
            {
                return this->Finish(std::nullopt, MapVariantOutcome::Failed, active->Name, "the configuration is too small to hold a map variant");
            }

            const std::span<std::byte> current{ configuration.subspan(k_ConfigurationMapVariantOffset, k_MapVariantSize) };
            const auto incomingHeader{ LayoutMapVariant::Read(active->Block) };
            const auto currentHeader{ LayoutMapVariant::Read(current) };

            if (!incomingHeader || !currentHeader)
            {
                return this->Finish(std::nullopt, MapVariantOutcome::Failed, active->Name, "the header of a map variant cannot be read");
            }

            if (const auto reason{ LayoutMapVariant::CheckCompatible(*incomingHeader, *currentHeader) })
            {
                return this->Finish(std::nullopt, MapVariantOutcome::Rejected, active->Name, *reason);
            }

            const std::vector<std::byte> backup(current.begin(), current.end());
            std::ranges::copy(active->Block.begin(), active->Block.begin() + k_MapVariantSize, current.begin());

            if (!std::ranges::equal(current, std::span<const std::byte>{ active->Block }.first(k_MapVariantSize)))
            {
                std::ranges::copy(backup, current.begin());
                return this->Finish(std::nullopt, MapVariantOutcome::Failed, active->Name, "the map variant did not read back equal");
            }

            m_Log.Message("[MapVariantService] INFO: Map variant \"{}\" written into the start configuration.", active->Name);
            return this->Finish(std::nullopt, MapVariantOutcome::AppliedAtLoad, active->Name, std::string{});
        }
        catch (const std::exception& exception)
        {
            return this->Finish(std::nullopt, MapVariantOutcome::Failed, active->Name, exception.what());
        }
    }

    auto MapVariantService::ApplyBetweenRounds(std::span<std::byte> runtime, std::span<std::byte> options, std::uint8_t roundState,
        std::uint8_t gameMode) -> void
    {
        if (!m_MapVariantStore.HasPending()) return;

        // It waits: never in the middle of a round, and only where the game globals hold a map variant.
        if (roundState != k_RoundStateEnded && roundState != k_RoundStateBetween) return;
        if (gameMode != k_GameModeMultiplayer) return;

        const auto request{ m_MapVariantStore.TakePending() };
        if (!request) return;

        const std::uint64_t id{ request->Id };

        // The bench chose another map variant after this request: it has nothing left to write.
        if (!m_MapVariantStore.IsCurrent(id)) return;

        std::vector<std::byte> backupRuntime{};
        std::vector<std::byte> backupOptions{};

        // An exception must not leave the hook: runtime and options go back to what they were.
        try
        {
            if (runtime.size() < k_MapVariantSize || options.size() < k_MapVariantSize)
            {
                this->Finish(id, MapVariantOutcome::Failed, request->Name, "the live map variant is too small");
                return;
            }

            // None brings back the map variant the match loaded with.
            const auto base{ m_MapVariantStore.GetBase() };
            const std::span<const std::byte> block{ request->MapVariant != nullptr ? std::span<const std::byte>{ request->MapVariant->Block }
                : (base != nullptr ? std::span<const std::byte>{ *base } : std::span<const std::byte>{}) };

            const auto incomingHeader{ LayoutMapVariant::Read(block) };
            const auto currentHeader{ LayoutMapVariant::Read(runtime) };

            if (!incomingHeader)
            {
                this->Finish(id, MapVariantOutcome::Rejected, request->Name, "the map variant this match loaded with is not known");
                return;
            }

            if (!currentHeader)
            {
                this->Finish(id, MapVariantOutcome::Failed, request->Name, "the header of the live map variant cannot be read");
                return;
            }

            if (const auto reason{ LayoutMapVariant::CheckCompatible(*incomingHeader, *currentHeader) })
            {
                this->Finish(id, MapVariantOutcome::Rejected, request->Name, *reason);
                return;
            }

            const std::vector<std::byte> composed{ LayoutMapVariant::ComposeRuntime(block, runtime) };
            backupRuntime.assign(runtime.begin(), runtime.begin() + k_MapVariantSize);
            backupOptions.assign(options.begin(), options.begin() + k_MapVariantSize);

            std::ranges::copy(composed, runtime.begin());
            std::ranges::copy(block.first(k_MapVariantSize), options.begin());

            if (!std::ranges::equal(runtime.first(k_MapVariantSize), composed) || !std::ranges::equal(options.first(k_MapVariantSize), block.first(k_MapVariantSize)))
            {
                std::ranges::copy(backupRuntime, runtime.begin());
                std::ranges::copy(backupOptions, options.begin());
                this->Finish(id, MapVariantOutcome::Failed, request->Name, "the map variant did not read back equal");
                return;
            }

            if (m_MapVariantStore.IsCurrent(id)) m_MapVariantStore.SetActive(request->MapVariant);
            m_Log.Message("[MapVariantService] INFO: Map variant \"{}\" written into the live map variant (placements in use: {}).",
                request->Name, incomingHeader->PlacementsInUse);
            this->Finish(id, MapVariantOutcome::Applied, request->Name, std::string{});
        }
        catch (const std::exception& exception)
        {
            if (!backupRuntime.empty()) std::ranges::copy(backupRuntime, runtime.begin());
            if (!backupOptions.empty()) std::ranges::copy(backupOptions, options.begin());
            this->Finish(id, MapVariantOutcome::Failed, request->Name, exception.what());
        }
    }

    auto MapVariantService::Finish(std::optional<std::uint64_t> requestId, MapVariantOutcome outcome, std::string_view name,
        std::string message) -> MapVariantOutcome
    {
        if (outcome == MapVariantOutcome::Rejected)
        {
            m_Log.Message("[MapVariantService] WARNING: Map variant \"{}\" does not fit the map ({}): the map variant stays as it was.",
                name, message);
        }
        else if (outcome == MapVariantOutcome::Failed)
        {
            m_Log.Message("[MapVariantService] ERROR: Map variant \"{}\" could not be used ({}).", name, message);
        }

        MapVariantResult result{ .Outcome = outcome, .Name = std::string{ name }, .Message = std::move(message) };
        if (requestId) m_MapVariantStore.SetResultOf(*requestId, std::move(result));
        else m_MapVariantStore.SetResult(std::move(result));
        return outcome;
    }
}