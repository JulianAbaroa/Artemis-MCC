export module Engine.Base.Preset.State:Preset;

import Blam.Layout.Game.Type;
import std;

export namespace Engine::Base::Preset::State
{
    // The active gametype preset, the catalog it was loaded with, and the base values saved for the match with the catalog they were
    // saved with (D264, D265).
    // note: Thread-safe, and data only. Written by the match setup service (MCC and game threads) and read by the Workbench. The
    // snapshots it hands out are immutable, so a reader keeps a consistent preset while another one is set.
    class PresetStore
    {
    private:
        using CompiledPreset = Blam::Layout::Game::Type::VariantPreset::CompiledPreset;
        using PresetBase = Blam::Layout::Game::Type::VariantPreset::PresetBase;
        using PresetCatalog = Blam::Layout::Game::Type::VariantPreset::PresetCatalog;

    public:
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

    private:
        mutable std::mutex m_Mutex{};
        std::shared_ptr<const CompiledPreset> m_Active{};
        std::shared_ptr<const PresetCatalog> m_Catalog{};
        std::shared_ptr<const PresetBase> m_Base{};
        std::shared_ptr<const PresetCatalog> m_BaseCatalog{};
    };
}