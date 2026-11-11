module Engine.Match.System;
import :MapVariant;

import Engine.Base.MapVariant.Type;
import Blam.Layout.Object.Type;
import std;

namespace
{
    namespace MapVariant = Blam::Layout::Object::Type::MapVariant;

    using Engine::Base::MapVariant::Type::ActiveMapVariant;
    using Engine::Base::MapVariant::Type::MapVariantResult;

    using Blam::Layout::Object::Type::Constant::k_ConfigurationMapVariantOffset;
    using Blam::Layout::Object::Type::Constant::k_MapVariantSize;
}

namespace Engine::Match::System
{
    auto MapVariantService::SetActive(std::string_view name) -> bool
    {
        try
        {
            auto block{ m_MatchConfigService.ReadMapVariant(name) };

            if (block.empty())
            {
                this->Finish(MapVariantOutcome::Failed, name, "the map variant of the configuration cannot be read");
                return false;
            }

            const auto header{ MapVariant::Read(block) };
            if (!header)
            {
                this->Finish(MapVariantOutcome::Failed, name, "the map variant has no header");
                return false;
            }

            m_Log.Message("[MapVariantService] INFO: Map variant \"{}\" (map id {}, {} quotas, {} placements) is active for the next start.",
                name, header->MapId, header->Quotas, header->PlacementsInUse);

            m_MapVariantStore.SetActive(std::make_shared<const ActiveMapVariant>(ActiveMapVariant{ .Name = std::string{ name },
                .Block = std::move(block) }));
            m_MapVariantStore.SetResult(MapVariantResult{});
            return true;
        }
        catch (const std::exception& exception)
        {
            this->Finish(MapVariantOutcome::Failed, name, exception.what());
            return false;
        }
    }

    auto MapVariantService::ClearActive() -> void
    {
        m_MapVariantStore.SetActive(nullptr);
        m_MapVariantStore.SetResult(MapVariantResult{});
    }

    auto MapVariantService::HasActive() const -> bool
    {
        return m_MapVariantStore.GetActive() != nullptr;
    }

    auto MapVariantService::ApplyToStart(std::span<std::byte> configuration) -> MapVariantOutcome
    {
        const auto active{ m_MapVariantStore.GetActive() };
        if (active == nullptr) return MapVariantOutcome::NoMap;

        try
        {
            if (configuration.size() < k_ConfigurationMapVariantOffset + k_MapVariantSize)
            {
                return this->Finish(MapVariantOutcome::Failed, active->Name, "the configuration is too small to hold a map variant");
            }

            const std::span<std::byte> current{ configuration.subspan(k_ConfigurationMapVariantOffset, k_MapVariantSize) };
            const auto incomingHeader{ MapVariant::Read(active->Block) };
            const auto currentHeader{ MapVariant::Read(current) };

            if (!incomingHeader || !currentHeader)
            {
                return this->Finish(MapVariantOutcome::Failed, active->Name, "the header of a map variant cannot be read");
            }

            if (const auto reason{ MapVariant::CheckCompatible(*incomingHeader, *currentHeader) })
            {
                return this->Finish(MapVariantOutcome::Rejected, active->Name, *reason);
            }

            const std::vector<std::byte> backup(current.begin(), current.end());
            std::ranges::copy(active->Block.begin(), active->Block.begin() + k_MapVariantSize, current.begin());

            if (!std::ranges::equal(current, std::span<const std::byte>{ active->Block }.first(k_MapVariantSize)))
            {
                std::ranges::copy(backup, current.begin());
                return this->Finish(MapVariantOutcome::Failed, active->Name, "the map variant did not read back equal");
            }

            m_Log.Message("[MapVariantService] INFO: Map variant \"{}\" written into the start configuration.", active->Name);
            return this->Finish(MapVariantOutcome::AppliedAtLoad, active->Name, std::string{});
        }
        catch (const std::exception& exception)
        {
            return this->Finish(MapVariantOutcome::Failed, active->Name, exception.what());
        }
    }

    auto MapVariantService::Finish(MapVariantOutcome outcome, std::string_view name, std::string message) -> MapVariantOutcome
    {
        if (outcome == MapVariantOutcome::Rejected)
        {
            m_Log.Message("[MapVariantService] WARNING: Map variant \"{}\" does not fit the map of the start ({}): the start keeps the map "
                "variant of its configuration.", name, message);
        }
        else if (outcome == MapVariantOutcome::Failed)
        {
            m_Log.Message("[MapVariantService] ERROR: Map variant \"{}\" could not be used ({}).", name, message);
        }

        m_MapVariantStore.SetResult(MapVariantResult{ .Outcome = outcome, .Name = std::string{ name }, .Message = std::move(message) });
        return outcome;
    }
}