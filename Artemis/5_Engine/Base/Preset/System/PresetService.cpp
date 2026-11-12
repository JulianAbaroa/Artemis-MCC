module Engine.Base.Preset.System;
import :Preset;

import Engine.Base.Preset.Type;
import Blam.Layout.Game.Type;
import std;

namespace
{
    namespace VariantPreset = Blam::Layout::Game::Type::VariantPreset;

    using Engine::Base::Preset::Type::PresetLivePlan;
    using Engine::Base::Preset::Type::PresetPick;
    using Engine::Base::Preset::Type::k_CatalogFileName;
    using Engine::Base::Preset::Type::k_ConfigurationVariantOffset;
    using Engine::Base::Preset::Type::k_MaxPresetNameLength;
    using Engine::Base::Preset::Type::k_PresetExtension;
    using Engine::Base::Preset::Type::k_PresetsFolder;

    constexpr std::string_view k_CatalogDiffers{ "the catalog of the preset differs from the one this match was saved with (run Tools/presets.py and start a new match)" };

    // return: Whether the name is one the compiler writes: lower case letters, digits and '_'.
    auto IsValidName(std::string_view name) -> bool
    {
        return !name.empty() && name.size() <= k_MaxPresetNameLength &&
            std::ranges::all_of(name, [](char c) { return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_'; });
    }

    // return: The text of a file, or nothing when it cannot be read.
    auto ReadText(const std::filesystem::path& file) -> std::optional<std::string>
    {
        std::ifstream stream{ file, std::ios::binary };
        if (!stream) return std::nullopt;

        return std::string{ std::istreambuf_iterator<char>{ stream }, std::istreambuf_iterator<char>{} };
    }
}

namespace Engine::Base::Preset::System
{
    auto PresetService::Resolve(std::string_view name) const -> std::expected<PresetPick, std::string>
    {
        if (name.empty()) return PresetPick{};

        const std::string directory{ m_SettingsStore.GetAppDataDirectory() };
        std::string failure{};

        if (!IsValidName(name)) failure = "the name is not valid ([a-z0-9_], up to 31 characters)";
        else if (directory.empty()) failure = "the storage folder is not set";

        const std::filesystem::path folder{ std::filesystem::path{ directory } / k_PresetsFolder };

        if (failure.empty())
        {
            const auto presetText{ ReadText(folder / (std::string{ name } + std::string{ k_PresetExtension })) };
            const auto catalogText{ ReadText(folder / k_CatalogFileName) };

            if (!presetText) failure = "the compiled file is missing (run Tools/presets.py)";
            else if (!catalogText) failure = "the compiled catalog is missing (run Tools/presets.py)";
            else
            {
                auto parsedPreset{ VariantPreset::ParsePreset(*presetText) };
                auto parsedCatalog{ VariantPreset::ParseCatalog(*catalogText) };

                if (!parsedPreset) failure = "the compiled file is wrong: " + parsedPreset.error();
                else if (!parsedCatalog) failure = "the compiled catalog is wrong: " + parsedCatalog.error();
                else if (parsedPreset->Name != name) failure = "the compiled file is named " + parsedPreset->Name;
                else
                {
                    return PresetPick{ .Name = std::string{ name },
                        .Preset = std::make_shared<const VariantPreset::CompiledPreset>(std::move(*parsedPreset)),
                        .Catalog = std::make_shared<const VariantPreset::PresetCatalog>(std::move(*parsedCatalog)) };
                }
            }
        }

        m_Log.Message("[PresetService] ERROR: Preset \"{}\" not set: {}.", name, failure);
        return std::unexpected(std::format("preset \"{}\": {}", name, failure));
    }

    auto PresetService::GetActive() const -> PresetPick
    {
        auto preset{ m_PresetStore.GetActive() };
        if (preset == nullptr) return PresetPick{};

        std::string name{ preset->Name };
        return PresetPick{ .Name = std::move(name), .Preset = std::move(preset), .Catalog = m_PresetStore.GetCatalog() };
    }

    auto PresetService::GetActiveName() const -> std::string
    {
        return m_PresetStore.GetActiveName();
    }

    auto PresetService::Adopt(const PresetPick& pick) -> void
    {
        if (pick.Preset != nullptr) m_PresetStore.SetCatalog(pick.Catalog);
        m_PresetStore.SetActive(pick.Preset);
    }

    auto PresetService::DropBase() -> void
    {
        m_PresetStore.SetBase(nullptr);
    }

    auto PresetService::Check(const PresetPick& pick) const -> std::optional<std::string>
    {
        const auto catalog{ m_PresetStore.GetBaseCatalog() };
        if (catalog != nullptr && pick.Catalog != nullptr && !(*pick.Catalog == *catalog)) return std::string{ k_CatalogDiffers };

        return std::nullopt;
    }

    auto PresetService::WriteToStart(std::span<std::byte> configuration, const PresetPick& pick) -> std::expected<bool, std::string>
    {
        if (pick.Preset == nullptr || pick.Catalog == nullptr) return false;
        if (configuration.size() <= k_ConfigurationVariantOffset) return std::unexpected("the configuration is too small to hold a game variant");

        const std::span<std::uint8_t> variant{ reinterpret_cast<std::uint8_t*>(configuration.data()) + k_ConfigurationVariantOffset,
            configuration.size() - k_ConfigurationVariantOffset };
        std::vector<std::uint8_t> backup{};

        // An exception must not reach MCC: the variant goes back to what it was.
        try
        {
            if (!VariantPreset::IsMegaloVariant(variant)) return false;

            auto base{ VariantPreset::SaveBase(*pick.Catalog, variant) };
            if (!base) return std::unexpected(base.error());

            auto writes{ VariantPreset::Plan(*pick.Catalog, *base, pick.Preset.get(), variant, VariantPreset::PresetPhase::Load) };
            if (!writes) return std::unexpected(writes.error().Reason);

            backup.assign(variant.begin(), variant.end());

            if (!VariantPreset::Apply(variant, *writes) || !VariantPreset::Verify(variant, *writes))
            {
                std::ranges::copy(backup, variant.begin());
                return std::unexpected("the preset did not read back equal");
            }

            m_PresetStore.SetBase(std::make_shared<const VariantPreset::PresetBase>(std::move(*base)), pick.Catalog);
            m_Log.Message("[PresetService] INFO: Preset \"{}\" written into the start configuration ({} changes).", pick.Name, writes->size());
            return true;
        }
        catch (const std::exception& exception)
        {
            if (!backup.empty()) std::ranges::copy(backup, variant.begin());
            return std::unexpected(exception.what());
        }
    }

    auto PresetService::PrepareLive(const PresetPick& pick, std::span<const std::uint8_t> variant) const -> std::expected<PresetLivePlan, std::string>
    {
        try
        {
            if (!VariantPreset::IsMegaloVariant(variant)) return std::unexpected("the game variant is not a Megalo variant");

            auto base{ m_PresetStore.GetBase() };
            auto catalog{ m_PresetStore.GetBaseCatalog() };
            bool isBaseNew{ false };

            if (base != nullptr)
            {
                // Everything is composed over the base with the catalog it was saved with.
                if (const auto reason{ this->Check(pick) }) return std::unexpected(*reason);
            }
            else if (pick.Preset == nullptr)
            {
                // None with nothing written this match: the variant already is the base.
                return PresetLivePlan{ .Pick = pick };
            }
            else
            {
                // First preset of a match loaded without one: the variant still holds the base values.
                catalog = pick.Catalog;
                auto saved{ VariantPreset::SaveBase(*catalog, variant) };
                if (!saved) return std::unexpected(saved.error());

                base = std::make_shared<const VariantPreset::PresetBase>(std::move(*saved));
                isBaseNew = true;
            }

            auto writes{ VariantPreset::Plan(*catalog, *base, pick.Preset.get(), variant, VariantPreset::PresetPhase::BetweenRounds) };
            if (!writes) return std::unexpected(writes.error().Reason);

            return PresetLivePlan{ .Pick = pick, .Base = std::move(base), .Catalog = std::move(catalog), .IsBaseNew = isBaseNew,
                .Writes = std::move(*writes) };
        }
        catch (const std::exception& exception)
        {
            return std::unexpected(exception.what());
        }
    }

    auto PresetService::CommitLive(const PresetLivePlan& plan, std::span<std::uint8_t> variant) const -> std::expected<void, std::string>
    {
        std::vector<std::uint8_t> backup{};

        // An exception must not leave the hook: the variant goes back to what it was.
        try
        {
            backup.assign(variant.begin(), variant.end());

            if (!VariantPreset::Apply(variant, plan.Writes) || !VariantPreset::Verify(variant, plan.Writes))
            {
                std::ranges::copy(backup, variant.begin());
                return std::unexpected("the preset did not read back equal");
            }

            return {};
        }
        catch (const std::exception& exception)
        {
            if (!backup.empty()) std::ranges::copy(backup, variant.begin());
            return std::unexpected(exception.what());
        }
    }

    auto PresetService::AdoptLive(const PresetLivePlan& plan) -> void
    {
        if (plan.IsBaseNew) m_PresetStore.SetBase(plan.Base, plan.Catalog);

        // The catalog of a plan composed over a base is the one of the base (checked equal to the one of the pick).
        if (plan.Pick.Preset != nullptr) m_PresetStore.SetCatalog(plan.Catalog != nullptr ? plan.Catalog : plan.Pick.Catalog);
        m_PresetStore.SetActive(plan.Pick.Preset);
    }
}