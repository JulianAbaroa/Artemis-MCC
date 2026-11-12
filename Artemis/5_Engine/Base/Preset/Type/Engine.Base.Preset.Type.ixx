export module Engine.Base.Preset.Type;

import Blam.Layout.Game.Type;
import std;

namespace
{
    using Blam::Layout::Game::Type::VariantPreset::CompiledPreset;
    using Blam::Layout::Game::Type::VariantPreset::PresetBase;
    using Blam::Layout::Game::Type::VariantPreset::PresetCatalog;
    using Blam::Layout::Game::Type::VariantPreset::PresetWrites;
}

export namespace Engine::Base::Preset::Type
{
    // Folder of the storage folder that holds the compiled presets (Tools/presets.py), the extension of their files and the file that
    // lists every writable field of the game variant (D265).
    inline constexpr std::string_view k_PresetsFolder{ "Presets" };
    inline constexpr std::string_view k_PresetExtension{ ".txt" };
    inline constexpr std::string_view k_CatalogFileName{ "_catalog.txt" };

    // Longest name of a preset, the same the compiler accepts.
    inline constexpr std::size_t k_MaxPresetNameLength{ 31 };

    // Offset of the unpacked game variant inside the configuration the engine starts with: the variant has the same layout there as in
    // the game globals (A19 sec. 5.2). It is 0x300, written in decimal because other unrelated offsets share that hex value.
    inline constexpr std::size_t k_ConfigurationVariantOffset{ 768 };

    // A preset read from the compiled files, ready to be part of a match setup (D286). A null preset asks for the base values of the
    // gametype (the name is then empty).
    struct PresetPick
    {
        std::string Name{};
        std::shared_ptr<const CompiledPreset> Preset{};
        std::shared_ptr<const PresetCatalog> Catalog{};
    };

    // A preset checked against the live variant of a match and not written yet.
    struct PresetLivePlan
    {
        PresetPick Pick{};
        // The base values the writes were composed over, and the catalog they were saved with. Null when the plan has no writes.
        std::shared_ptr<const PresetBase> Base{};
        std::shared_ptr<const PresetCatalog> Catalog{};
        // Whether Base was saved from the variant for this plan (the first preset of a match loaded without one).
        bool IsBaseNew{ false };
        PresetWrites Writes{};
    };
}