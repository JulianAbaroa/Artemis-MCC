module Engine.Base.MapVariant.System;
import :MapVariant;

import Engine.Base.MapVariant.Type;
import Blam.Layout.Object.Type;
import std;

namespace
{
    namespace LayoutMapVariant = Blam::Layout::Object::Type::MapVariant;

    using Engine::Base::MapVariant::Type::ActiveMapVariant;
    using Engine::Base::MapVariant::Type::MapVariantLivePlan;

    using Blam::Layout::Object::Type::Constant::k_ConfigurationMapVariantOffset;
    using Blam::Layout::Object::Type::Constant::k_MapVariantSize;
}

namespace Engine::Base::MapVariant::System
{
    auto MapVariantService::Resolve(std::string_view name, std::vector<std::byte> block) const
        -> std::expected<std::shared_ptr<const ActiveMapVariant>, std::string>
    {
        if (name.empty()) return std::shared_ptr<const ActiveMapVariant>{};

        try
        {
            // A block that cannot be used leaves the active map variant as it was.
            if (block.empty() || !LayoutMapVariant::Read(block))
            {
                m_Log.Message("[MapVariantService] ERROR: Map variant \"{}\" could not be used (the map variant of the configuration cannot be read).", name);
                return std::unexpected(std::format("map variant \"{}\": the map variant of the configuration cannot be read", name));
            }

            return std::make_shared<const ActiveMapVariant>(ActiveMapVariant{ .Name = std::string{ name }, .Block = std::move(block) });
        }
        catch (const std::exception& exception)
        {
            m_Log.Message("[MapVariantService] ERROR: Map variant \"{}\" could not be used ({}).", name, exception.what());
            return std::unexpected(std::format("map variant \"{}\": {}", name, exception.what()));
        }
    }

    auto MapVariantService::GetActive() const -> std::shared_ptr<const ActiveMapVariant>
    {
        return m_MapVariantStore.GetActive();
    }

    auto MapVariantService::GetActiveName() const -> std::string
    {
        return m_MapVariantStore.GetActiveName();
    }

    auto MapVariantService::Adopt(std::shared_ptr<const ActiveMapVariant> mapVariant) -> void
    {
        m_MapVariantStore.SetActive(std::move(mapVariant));
    }

    auto MapVariantService::Check(const ActiveMapVariant& mapVariant) const -> std::optional<std::string>
    {
        const auto base{ m_MapVariantStore.GetBase() };
        if (base == nullptr) return std::nullopt;

        const auto incomingHeader{ LayoutMapVariant::Read(mapVariant.Block) };
        const auto baseHeader{ LayoutMapVariant::Read(*base) };
        if (!incomingHeader || !baseHeader) return std::nullopt;

        return LayoutMapVariant::CheckCompatible(*incomingHeader, *baseHeader);
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
        }
        catch (const std::exception& exception)
        {
            m_Log.Message("[MapVariantService] ERROR: Failed to begin the start ({}).", exception.what());
        }
    }

    auto MapVariantService::WriteToStart(std::span<std::byte> configuration, const ActiveMapVariant& mapVariant) const -> std::expected<void, std::string>
    {
        try
        {
            if (configuration.size() < k_ConfigurationMapVariantOffset + k_MapVariantSize)
            {
                return std::unexpected("the configuration is too small to hold a map variant");
            }

            const std::span<std::byte> current{ configuration.subspan(k_ConfigurationMapVariantOffset, k_MapVariantSize) };
            const auto incomingHeader{ LayoutMapVariant::Read(mapVariant.Block) };
            const auto currentHeader{ LayoutMapVariant::Read(current) };

            if (!incomingHeader || !currentHeader) return std::unexpected("the header of a map variant cannot be read");

            if (const auto reason{ LayoutMapVariant::CheckCompatible(*incomingHeader, *currentHeader) }) return std::unexpected(*reason);

            const std::vector<std::byte> backup(current.begin(), current.end());
            std::ranges::copy(mapVariant.Block.begin(), mapVariant.Block.begin() + k_MapVariantSize, current.begin());

            if (!std::ranges::equal(current, std::span<const std::byte>{ mapVariant.Block }.first(k_MapVariantSize)))
            {
                std::ranges::copy(backup, current.begin());
                return std::unexpected("the map variant did not read back equal");
            }

            m_Log.Message("[MapVariantService] INFO: Map variant \"{}\" written into the start configuration.", mapVariant.Name);
            return {};
        }
        catch (const std::exception& exception)
        {
            return std::unexpected(exception.what());
        }
    }

    auto MapVariantService::PrepareLive(std::shared_ptr<const ActiveMapVariant> mapVariant, std::span<const std::byte> runtime) const
        -> std::expected<MapVariantLivePlan, std::string>
    {
        try
        {
            if (runtime.size() < k_MapVariantSize) return std::unexpected("the live map variant is too small");

            // None brings back the map variant the match loaded with.
            const auto base{ m_MapVariantStore.GetBase() };
            const std::span<const std::byte> block{ mapVariant != nullptr ? std::span<const std::byte>{ mapVariant->Block }
                : (base != nullptr ? std::span<const std::byte>{ *base } : std::span<const std::byte>{}) };

            const auto incomingHeader{ LayoutMapVariant::Read(block) };
            const auto currentHeader{ LayoutMapVariant::Read(runtime) };

            if (!incomingHeader) return std::unexpected("the map variant this match loaded with is not known");
            if (!currentHeader) return std::unexpected("the header of the live map variant cannot be read");

            if (const auto reason{ LayoutMapVariant::CheckCompatible(*incomingHeader, *currentHeader) }) return std::unexpected(*reason);

            // The labels keep the index the match built its Megalo object lists with: a map variant with the same labels in another order
            // crashes the next round reset. The runtime and the options get the same remapped block, so a copy of one over the other mid-match
            // cannot bring the old order back.
            const auto remap{ LayoutMapVariant::RemapLabels(runtime, block) };
            if (!remap) return std::unexpected("labels: " + remap.error());

            return MapVariantLivePlan{ .Pick = std::move(mapVariant), .Block = remap->Block,
                .Composed = LayoutMapVariant::ComposeRuntime(remap->Block, runtime), .PlacementsInUse = incomingHeader->PlacementsInUse,
                .LabelsMoved = remap->Moved, .LabelsAppended = remap->Appended, .LabelsKept = remap->Kept };
        }
        catch (const std::exception& exception)
        {
            return std::unexpected(exception.what());
        }
    }

    auto MapVariantService::CommitLive(const MapVariantLivePlan& plan, std::span<std::byte> runtime, std::span<std::byte> options) const
        -> std::expected<void, std::string>
    {
        std::vector<std::byte> backupRuntime{};
        std::vector<std::byte> backupOptions{};

        // An exception must not leave the hook: runtime and options go back to what they were.
        try
        {
            if (runtime.size() < k_MapVariantSize || options.size() < k_MapVariantSize) return std::unexpected("the live map variant is too small");

            backupRuntime.assign(runtime.begin(), runtime.begin() + k_MapVariantSize);
            backupOptions.assign(options.begin(), options.begin() + k_MapVariantSize);

            std::ranges::copy(plan.Composed, runtime.begin());
            std::ranges::copy(plan.Block, options.begin());

            if (!std::ranges::equal(runtime.first(k_MapVariantSize), plan.Composed) || !std::ranges::equal(options.first(k_MapVariantSize), plan.Block))
            {
                std::ranges::copy(backupRuntime, runtime.begin());
                std::ranges::copy(backupOptions, options.begin());
                return std::unexpected("the map variant did not read back equal");
            }

            m_Log.Message("[MapVariantService] INFO: Map variant \"{}\" written into the live map variant (placements in use: {}).",
                plan.Pick != nullptr ? plan.Pick->Name : std::string{ "of the load" }, plan.PlacementsInUse);
            if (plan.LabelsMoved > 0 || plan.LabelsAppended > 0)
            {
                m_Log.Message("[MapVariantService] INFO: labels remapped: {} moved, {} appended ({} of the match kept).", plan.LabelsMoved, plan.LabelsAppended,
                    plan.LabelsKept);
            }
            return {};
        }
        catch (const std::exception& exception)
        {
            if (!backupRuntime.empty()) std::ranges::copy(backupRuntime, runtime.begin());
            if (!backupOptions.empty()) std::ranges::copy(backupOptions, options.begin());
            return std::unexpected(exception.what());
        }
    }
}