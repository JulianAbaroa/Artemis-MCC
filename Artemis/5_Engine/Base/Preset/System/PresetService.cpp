module Engine.Base.Preset.System;
import :Preset;

import Engine.Base.Preset.Type;
import Blam.Layout.Game.Type;
import std;

namespace
{
    namespace VariantPreset = Blam::Layout::Game::Type::VariantPreset;

    using Blam::Layout::Game::Type::Offset::k_RoundStateBetween;
    using Blam::Layout::Game::Type::Offset::k_RoundStateEnded;
    using Blam::Layout::Game::Type::Offset::k_VariantModeFirst;
    using Blam::Layout::Game::Type::Offset::k_VariantModeLast;
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
    auto PresetService::Load(std::string_view name, std::shared_ptr<const VariantPreset::CompiledPreset>& preset,
        std::shared_ptr<const VariantPreset::PresetCatalog>& catalog) -> bool
    {
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

        return true;
    }

    auto PresetService::SetActive(std::string_view name) -> bool
    {
        std::shared_ptr<const VariantPreset::CompiledPreset> preset{};
        std::shared_ptr<const VariantPreset::PresetCatalog> catalog{};
        if (!this->Load(name, preset, catalog)) return false;

        // The choice of the bench replaces a request of the bridge that still waits for a round break.
        m_PresetStore.ClearPending();
        m_PresetStore.SetCatalog(std::move(catalog));
        m_PresetStore.SetActive(std::move(preset));
        m_PresetStore.SetResult(PresetResult{});
        m_Log.Message("[PresetService] INFO: Preset \"{}\" is the active one.", name);
        return true;
    }

    auto PresetService::Request(std::string_view name, bool isInMatch) -> bool
    {
        const std::uint64_t id{ m_PresetStore.BeginRequest() };

        std::shared_ptr<const VariantPreset::CompiledPreset> preset{};
        std::shared_ptr<const VariantPreset::PresetCatalog> catalog{};

        // A request whose files cannot be read leaves the request waiting from before as it was.
        if (!name.empty() && !this->Load(name, preset, catalog)) return false;

        if (!isInMatch)
        {
            m_PresetStore.ClearPending();
            if (preset != nullptr) m_PresetStore.SetCatalog(std::move(catalog));
            m_PresetStore.SetActive(std::move(preset));
            m_PresetStore.SetResultOf(id, PresetResult{ .Outcome = PresetOutcome::AppliedAtLoad, .Name = std::string{ name } });
            m_Log.Message("[PresetService] INFO: Preset \"{}\" will be written at the next start.", name);
            return true;
        }

        m_PresetStore.SetPending(PresetStore::PendingRequest{ .Id = id, .Name = std::string{ name }, .Preset = std::move(preset),
            .Catalog = std::move(catalog) });
        m_PresetStore.SetResultOf(id, PresetResult{ .Outcome = PresetOutcome::Pending, .Name = std::string{ name } });
        m_Log.Message("[PresetService] INFO: Preset \"{}\" requested, it waits for a round break.", name);
        return true;
    }

    auto PresetService::CancelPending() -> void
    {
        const auto request{ m_PresetStore.TakePending() };
        if (!request) return;

        m_PresetStore.SetResultOf(request->Id, PresetResult{});
        m_Log.Message("[PresetService] INFO: Preset \"{}\" request dropped.", request->Name);
    }

    auto PresetService::BeginStart() -> void
    {
        // The base belongs to the match that ended: the start holds the values of this one.
        m_PresetStore.SetBase(nullptr);

        // A request no round break served (the match ended first) is the wanted preset of this start, or none.
        const auto request{ m_PresetStore.TakePending() };
        if (!request) return;

        if (request->Preset != nullptr) m_PresetStore.SetCatalog(request->Catalog);
        m_PresetStore.SetActive(request->Preset);
        if (request->Preset == nullptr)
        {
            m_PresetStore.SetResultOf(request->Id, PresetResult{ .Outcome = PresetOutcome::AppliedAtLoad });
        }
        m_Log.Message("[PresetService] INFO: Preset \"{}\" requested in the last match is taken by this start.", request->Name);
    }

    auto PresetService::ClearActive() -> void
    {
        m_PresetStore.ClearPending();
        m_PresetStore.SetActive(nullptr);
        m_PresetStore.SetResult(PresetResult{});
        m_Log.Message("[PresetService] INFO: No active preset.");
    }

    auto PresetService::HasActive() const -> bool
    {
        return m_PresetStore.GetActive() != nullptr;
    }

    auto PresetService::HasPending() const -> bool
    {
        return m_PresetStore.HasPending();
    }

    auto PresetService::Finish(std::optional<std::uint64_t> requestId, PresetOutcome outcome, std::string_view name, std::string message) -> PresetOutcome
    {
        if (outcome == PresetOutcome::Failed) m_Log.Message("[PresetService] ERROR: Preset \"{}\" not written: {}.", name, message);
        else if (outcome != PresetOutcome::AppliedAtLoad && outcome != PresetOutcome::Applied) m_Log.Message("[PresetService] WARNING: Preset \"{}\" not written: {}.", name, message);

        PresetResult result{ .Outcome = outcome, .Name = std::string{ name }, .Message = std::move(message) };
        if (requestId) m_PresetStore.SetResultOf(*requestId, std::move(result));
        else m_PresetStore.SetResult(std::move(result));
        return outcome;
    }

    auto PresetService::ApplyToStart(std::span<std::byte> configuration) -> PresetOutcome
    {
        const auto preset{ m_PresetStore.GetActive() };
        const auto catalog{ m_PresetStore.GetCatalog() };

        if (preset == nullptr || catalog == nullptr) return PresetOutcome::NoPreset;
        if (configuration.size() <= k_ConfigurationVariantOffset)
        {
            return this->Finish(std::nullopt, PresetOutcome::Failed, preset->Name, "the configuration is too small to hold a game variant");
        }

        const std::span<std::uint8_t> variant{ reinterpret_cast<std::uint8_t*>(configuration.data()) + k_ConfigurationVariantOffset,
            configuration.size() - k_ConfigurationVariantOffset };
        std::vector<std::uint8_t> backup{};

        // An exception must not reach MCC: the variant goes back to what it was and the start goes on without the preset.
        try
        {
            if (!VariantPreset::IsMegaloVariant(variant))
            {
                return this->Finish(std::nullopt, PresetOutcome::NoPreset, preset->Name, "the game variant is not a Megalo variant");
            }

            auto base{ VariantPreset::SaveBase(*catalog, variant) };
            if (!base) return this->Finish(std::nullopt, PresetOutcome::Failed, preset->Name, base.error());

            auto writes{ VariantPreset::Plan(*catalog, *base, preset.get(), variant, VariantPreset::PresetPhase::Load) };
            if (!writes) return this->Finish(std::nullopt, PresetOutcome::Rejected, preset->Name, writes.error().Reason);

            backup.assign(variant.begin(), variant.end());

            if (!VariantPreset::Apply(variant, *writes) || !VariantPreset::Verify(variant, *writes))
            {
                std::ranges::copy(backup, variant.begin());
                return this->Finish(std::nullopt, PresetOutcome::Failed, preset->Name, "the preset did not read back equal");
            }

            m_PresetStore.SetBase(std::make_shared<const VariantPreset::PresetBase>(std::move(*base)), catalog);
            m_Log.Message("[PresetService] INFO: Preset \"{}\" written into the start configuration ({} changes).", preset->Name, writes->size());
            return this->Finish(std::nullopt, PresetOutcome::AppliedAtLoad, preset->Name, std::string{});
        }
        catch (const std::exception& exception)
        {
            if (!backup.empty()) std::ranges::copy(backup, variant.begin());
            return this->Finish(std::nullopt, PresetOutcome::Failed, preset->Name, exception.what());
        }
    }

    auto PresetService::ApplyBetweenRounds(std::span<std::uint8_t> variant, std::uint8_t roundState, std::uint8_t gameMode) -> void
    {
        if (!m_PresetStore.HasPending()) return;

        // It waits: never in the middle of a round, and only where the game globals hold a variant.
        if (roundState != k_RoundStateEnded && roundState != k_RoundStateBetween) return;
        if (gameMode < k_VariantModeFirst || gameMode > k_VariantModeLast) return;

        const auto request{ m_PresetStore.TakePending() };
        if (!request) return;

        const std::uint64_t id{ request->Id };
        std::vector<std::uint8_t> backup{};

        // An exception must not leave the hook: the variant goes back to what it was.
        try
        {
            if (!VariantPreset::IsMegaloVariant(variant))
            {
                this->Finish(id, PresetOutcome::Rejected, request->Name, "the game variant is not a Megalo variant");
                return;
            }

            auto base{ m_PresetStore.GetBase() };
            auto catalog{ m_PresetStore.GetBaseCatalog() };

            if (base != nullptr)
            {
                // Everything is composed over the base with the catalog it was saved with.
                if (request->Catalog != nullptr && !(*request->Catalog == *catalog))
                {
                    this->Finish(id, PresetOutcome::Rejected, request->Name,
                        "the catalog of the preset differs from the one this match was saved with (run Tools/presets.py and start a new match)");
                    return;
                }
            }
            else if (request->Preset == nullptr)
            {
                // None with nothing written this match: the variant already is the base.
                m_PresetStore.SetActive(nullptr);
                this->Finish(id, PresetOutcome::Applied, request->Name, std::string{});
                return;
            }
            else
            {
                // First request of a match loaded without a preset: the variant still holds the base values.
                catalog = request->Catalog;
                auto saved{ VariantPreset::SaveBase(*catalog, variant) };
                if (!saved)
                {
                    this->Finish(id, PresetOutcome::Failed, request->Name, saved.error());
                    return;
                }

                base = std::make_shared<const VariantPreset::PresetBase>(std::move(*saved));
                m_PresetStore.SetBase(base, catalog);
            }

            auto writes{ VariantPreset::Plan(*catalog, *base, request->Preset.get(), variant, VariantPreset::PresetPhase::BetweenRounds) };
            if (!writes)
            {
                this->Finish(id, PresetOutcome::Rejected, request->Name, writes.error().Reason);
                return;
            }

            backup.assign(variant.begin(), variant.end());

            if (!VariantPreset::Apply(variant, *writes) || !VariantPreset::Verify(variant, *writes))
            {
                std::ranges::copy(backup, variant.begin());
                this->Finish(id, PresetOutcome::Failed, request->Name, "the preset did not read back equal");
                return;
            }

            if (request->Preset != nullptr) m_PresetStore.SetCatalog(catalog);
            m_PresetStore.SetActive(request->Preset);
            m_Log.Message("[PresetService] INFO: Preset \"{}\" written into the live variant ({} changes).", request->Name, writes->size());
            this->Finish(id, PresetOutcome::Applied, request->Name, std::string{});
        }
        catch (const std::exception& exception)
        {
            if (!backup.empty()) std::ranges::copy(backup, variant.begin());
            this->Finish(id, PresetOutcome::Failed, request->Name, exception.what());
        }
    }
}