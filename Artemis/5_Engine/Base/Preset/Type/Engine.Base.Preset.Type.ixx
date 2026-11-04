export module Engine.Base.Preset.Type;

import std;

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

    // What the last load did with the active preset.
    enum class PresetOutcome : std::uint8_t
    {
        // No load happened since the result was cleared.
        None,
        // The load had no preset to write, or its variant is not one a preset can be written to.
        NoPreset,
        // The preset was written into the start and read back equal.
        AppliedAtLoad,
        // The request was refused (a trait set the script does not have): the start went on without the preset.
        Rejected,
        // The preset could not be loaded or written: the start went on without it.
        Failed,
        // A request made during a match waits for the round to end.
        Pending,
        // The request was written into the live variant between rounds and read back equal.
        Applied,
    };

    // Result of the last load or of the last request to set the active preset.
    // note: A request made in the lobby ends as AppliedAtLoad (written at the next start). One made in a match is Pending until the next
    // round break, then Applied, Rejected or Failed.
    struct PresetResult
    {
        PresetOutcome Outcome{ PresetOutcome::None };
        std::string Name{};
        // Why it was not applied. Empty when it was.
        std::string Message{};
    };
}