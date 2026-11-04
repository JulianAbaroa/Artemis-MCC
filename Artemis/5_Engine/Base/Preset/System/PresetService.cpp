module Engine.Base.Preset.System;
import :Preset;

import Engine.Base.Preset.Type;
import Blam.Layout.Game.Type;
import std;

namespace
{
    namespace VariantPreset = Blam::Layout::Game::Type::VariantPreset;

    using Engine::Base::Preset::Type::PresetOutcome;
    using Engine::Base::Preset::Type::PresetResult;
    using Engine::Base::Preset::Type::k_CatalogFileName;
    using Engine::Base::Preset::Type::k_ConfigurationVariantOffset;
    using Engine::Base::Preset::Type::k_MaxPresetNameLength;
    using Engine::Base::Preset::Type::k_PresetExtension;
    using Engine::Base::Preset::Type::k_PresetsFolder;

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
    auto PresetService::SetActive(std::string_view name) -> bool
    {
        const std::string directory{ m_SettingsStore.GetAppDataDirectory() };
        std::string failure{};

        if (!IsValidName(name)) failure = "the name is not valid ([a-z0-9_], up to 31 characters)";
        else if (directory.empty()) failure = "the storage folder is not set";

        const std::filesystem::path folder{ std::filesystem::path{ directory } / k_PresetsFolder };
        std::shared_ptr<const VariantPreset::CompiledPreset> preset{};
        std::shared_ptr<const VariantPreset::PresetCatalog> catalog{};

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
                    preset = std::make_shared<const VariantPreset::CompiledPreset>(std::move(*parsedPreset));
                    catalog = std::make_shared<const VariantPreset::PresetCatalog>(std::move(*parsedCatalog));
                }
            }
        }

        if (!failure.empty())
        {
            m_Log.Message("[PresetService] ERROR: Preset \"{}\" not set: {}.", name, failure);
            m_PresetStore.SetResult(PresetResult{ .Outcome = PresetOutcome::Failed, .Name = std::string{ name }, .Message = failure });
            return false;
        }

        m_PresetStore.SetCatalog(std::move(catalog));
        m_PresetStore.SetActive(std::move(preset));
        m_PresetStore.SetResult(PresetResult{});
        m_Log.Message("[PresetService] INFO: Preset \"{}\" is the active one.", name);
        return true;
    }

    auto PresetService::ClearActive() -> void
    {
        m_PresetStore.SetActive(nullptr);
        m_PresetStore.SetResult(PresetResult{});
        m_Log.Message("[PresetService] INFO: No active preset.");
    }

    auto PresetService::HasActive() const -> bool
    {
        return m_PresetStore.GetActive() != nullptr;
    }

    auto PresetService::Finish(PresetOutcome outcome, std::string_view name, std::string message) -> PresetOutcome
    {
        if (outcome == PresetOutcome::Failed) m_Log.Message("[PresetService] ERROR: Preset \"{}\" not written: {}.", name, message);
        else if (outcome != PresetOutcome::AppliedAtLoad) m_Log.Message("[PresetService] WARNING: Preset \"{}\" not written: {}.", name, message);

        m_PresetStore.SetResult(PresetResult{ .Outcome = outcome, .Name = std::string{ name }, .Message = std::move(message) });
        return outcome;
    }

    auto PresetService::ApplyToStart(std::span<std::byte> configuration) -> PresetOutcome
    {
        const auto preset{ m_PresetStore.GetActive() };
        const auto catalog{ m_PresetStore.GetCatalog() };

        if (preset == nullptr || catalog == nullptr) return PresetOutcome::NoPreset;
        if (configuration.size() <= k_ConfigurationVariantOffset)
        {
            return this->Finish(PresetOutcome::Failed, preset->Name, "the configuration is too small to hold a game variant");
        }

        const std::span<std::uint8_t> variant{ reinterpret_cast<std::uint8_t*>(configuration.data()) + k_ConfigurationVariantOffset,
            configuration.size() - k_ConfigurationVariantOffset };
        std::vector<std::uint8_t> backup{};

        // An exception must not reach MCC: the variant goes back to what it was and the start goes on without the preset.
        try
        {
            if (!VariantPreset::IsMegaloVariant(variant))
            {
                return this->Finish(PresetOutcome::NoPreset, preset->Name, "the game variant is not a Megalo variant");
            }

            auto base{ VariantPreset::SaveBase(*catalog, variant) };
            if (!base) return this->Finish(PresetOutcome::Failed, preset->Name, base.error());

            auto writes{ VariantPreset::Plan(*catalog, *base, preset.get(), variant, VariantPreset::PresetPhase::Load) };
            if (!writes) return this->Finish(PresetOutcome::Rejected, preset->Name, writes.error().Reason);

            backup.assign(variant.begin(), variant.end());

            if (!VariantPreset::Apply(variant, *writes) || !VariantPreset::Verify(variant, *writes))
            {
                std::ranges::copy(backup, variant.begin());
                return this->Finish(PresetOutcome::Failed, preset->Name, "the preset did not read back equal");
            }

            m_PresetStore.SetBase(std::make_shared<const VariantPreset::PresetBase>(std::move(*base)));
            m_Log.Message("[PresetService] INFO: Preset \"{}\" written into the start configuration ({} changes).", preset->Name, writes->size());
            return this->Finish(PresetOutcome::AppliedAtLoad, preset->Name, std::string{});
        }
        catch (const std::exception& exception)
        {
            if (!backup.empty()) std::ranges::copy(backup, variant.begin());
            return this->Finish(PresetOutcome::Failed, preset->Name, exception.what());
        }
    }
}