export module Engine.Base.Preset.System:Preset;

import Common.Log.System;
import Services.Settings.State;
import Engine.Base.Preset.Type;
import Engine.Base.Preset.State;
import std;

export namespace Engine::Base::Preset::System
{
    // Reads the compiled gametype presets from the storage folder and writes one into a game variant: the one of the configuration the
    // engine starts with, or the live one at a round break (D264, D265). When and in which order it happens is decided by
    // MatchSetupService (D286), which also keeps the requests and the results.
    // note: The files are <Storage Folder>/Presets/<name>.txt and _catalog.txt, written by Tools/presets.py. Nothing here touches the game
    // memory: the writing methods work on buffers they are given.
    class PresetService
    {
    private:
        using Log = Common::Log::System::Log;
        using SettingsStore = Services::Settings::State::SettingsStore;

        using PresetLivePlan = Engine::Base::Preset::Type::PresetLivePlan;
        using PresetPick = Engine::Base::Preset::Type::PresetPick;
        using PresetStore = Engine::Base::Preset::State::PresetStore;

    public:
        PresetService(Log& log, SettingsStore& settingsStore, PresetStore& presetStore) :
            m_Log(log), m_SettingsStore(settingsStore), m_PresetStore(presetStore) {}
        ~PresetService() = default;

        PresetService(const PresetService&) = delete;
        auto operator=(const PresetService&) -> PresetService& = delete;

        // Reads the compiled preset and the catalog from disk. The files are read every time, so an edited preset is picked up by the next
        // call. Nothing in the store changes.
        // param name: Empty for the base values of the gametype: the pick has no preset.
        // return: The pick, or why the name is not valid or a file is missing or wrong (also logged as an ERROR).
        // note: Any thread except the game thread: it reads the disk.
        auto Resolve(std::string_view name) const -> std::expected<PresetPick, std::string>;

        // return: The active preset with its catalog, or a pick with no preset when none is active.
        auto GetActive() const -> PresetPick;

        // return: The name of the active preset, empty when there is none.
        auto GetActiveName() const -> std::string;

        // Makes the pick the active preset, which every start writes. A pick with no preset clears the active one.
        auto Adopt(const PresetPick& pick) -> void;

        // Drops the base values saved for the match that ended: the start holds the ones of the new match.
        auto DropBase() -> void;

        // return: Why the pick can never be written into the match that is running, or nothing. Only its catalog is compared: it has to
        // be the one the base of the match was saved with. Cheap and without memory access, so the thread that asks can check it.
        auto Check(const PresetPick& pick) const -> std::optional<std::string>;

        // Writes the preset into the game variant of a start configuration and reads it back: saves the base value of every field of the
        // catalog (with the catalog), plans the preset for the load, applies it and verifies it.
        // param configuration: The whole configuration the engine starts with. It has to be a copy: the one of MCC is never written.
        // return: True when written, false when the variant is not a Megalo one (nothing to write there), or why it could not be. A
        // failure leaves the configuration as it was.
        // note: MCC thread of the start. Never throws.
        auto WriteToStart(std::span<std::byte> configuration, const PresetPick& pick) -> std::expected<bool, std::string>;

        // Plans the preset against the live game variant of a match without writing anything. The base values come from the store, or
        // are saved from the variant when this is the first preset of a match loaded without one.
        // param variant: The live game variant (GameGlobals + 0x14E8).
        // return: The plan, or the reason it is refused: not a Megalo variant, another catalog than the base, a field only a load can
        // change, a trait set the script lacks.
        // note: Game thread. Never throws.
        auto PrepareLive(const PresetPick& pick, std::span<const std::uint8_t> variant) const -> std::expected<PresetLivePlan, std::string>;

        // Writes a plan into the live variant and reads it back. A write that does not read back equal restores the variant.
        // return: Nothing, or why it failed (the variant is then as it was).
        // note: Game thread. Never throws. The store does not change: AdoptLive does it once everything else of the setup was written.
        auto CommitLive(const PresetLivePlan& plan, std::span<std::uint8_t> variant) const -> std::expected<void, std::string>;

        // Keeps what a committed plan changed in the store: the base saved for it, and the pick as the active preset.
        auto AdoptLive(const PresetLivePlan& plan) -> void;

    private:
        Log& m_Log;
        SettingsStore& m_SettingsStore;
        PresetStore& m_PresetStore;
    };
}