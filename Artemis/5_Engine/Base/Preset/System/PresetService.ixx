export module Engine.Base.Preset.System:Preset;

import Common.Log.System;
import Services.Settings.State;
import Engine.Base.Preset.Type;
import Engine.Base.Preset.State;
import std;

export namespace Engine::Base::Preset::System
{
    // Loads the compiled gametype presets from the storage folder and writes the active one into the configuration the engine starts with
    // (D264, D265).
    // note: The files are <Storage Folder>/Presets/<name>.txt and _catalog.txt, written by Tools/presets.py. Nothing here touches the game
    // memory: ApplyToStart works on a buffer it is given.
    class PresetService
    {
    private:
        using Log = Common::Log::System::Log;
        using SettingsStore = Services::Settings::State::SettingsStore;

        using PresetOutcome = Engine::Base::Preset::Type::PresetOutcome;
        using PresetStore = Engine::Base::Preset::State::PresetStore;

    public:
        PresetService(Log& log, SettingsStore& settingsStore, PresetStore& presetStore) :
            m_Log(log), m_SettingsStore(settingsStore), m_PresetStore(presetStore) {}
        ~PresetService() = default;

        PresetService(const PresetService&) = delete;
        auto operator=(const PresetService&) -> PresetService& = delete;

        // Reads the compiled preset and the catalog from disk and makes the preset the active one. The files are read again every time, so
        // an edited preset is picked up by the next call.
        // return: False when the name is not valid or a file is missing or wrong. The active preset does not change and the result of the
        // store says why.
        auto SetActive(std::string_view name) -> bool;

        // Makes the next loads write no preset.
        auto ClearActive() -> void;

        // return: Whether a preset is active.
        auto HasActive() const -> bool;

        // Writes the active preset into the game variant of a start configuration and reads it back: saves the base value of every field
        // of the catalog, plans the preset for the load, applies it and verifies it.
        // param configuration: The whole configuration the engine starts with. It has to be a copy: the one of MCC is never written.
        // return: The outcome, also left in the store. Anything but AppliedAtLoad leaves the configuration as it was.
        // note: Called by the start hook on the MCC thread. Never throws.
        auto ApplyToStart(std::span<std::byte> configuration) -> PresetOutcome;

    private:
        Log& m_Log;
        SettingsStore& m_SettingsStore;
        PresetStore& m_PresetStore;

        // Leaves the outcome and its reason in the store and logs it.
        auto Finish(PresetOutcome outcome, std::string_view name, std::string message) -> PresetOutcome;
    };
}