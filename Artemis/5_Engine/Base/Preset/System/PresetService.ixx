export module Engine.Base.Preset.System:Preset;

import Common.Log.System;
import Services.Settings.State;
import Engine.Base.Preset.Type;
import Engine.Base.Preset.State;
import Blam.Layout.Game.Type;
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

        // Asks for a preset from the bridge or the bench, any thread. An empty name asks for none.
        // param isInMatch: In the lobby it becomes the active preset at once and the next start writes it (AppliedAtLoad). In a match the
        // files are read now and the request waits for a round break (Pending): ApplyBetweenRounds serves it.
        // return: False when the files could not be read. The result of the store says why, the active preset does not change.
        auto Request(std::string_view name, bool isInMatch) -> bool;

        // return: Whether a request waits for a round break.
        auto HasPending() const -> bool;

        // Starts the per-start state, for every start with or without a preset: drops the base saved for the match that ended and hands a
        // request no round break served over to the active preset (or clears it when the request was for none). ApplyToStart only writes.
        // note: Called by the start hook on the MCC thread, before HasActive.
        auto BeginStart() -> void;

        // Drops the request waiting, when the client that made it is gone.
        auto CancelPending() -> void;

        // Serves the request waiting, if any, into the live game variant. Nothing happens unless the round is ended or between rounds and
        // the game mode has a variant. The base values are saved from the variant on the first request of the match, before anything is
        // written. A request the plan refuses (a field only a load can change, a trait set the script lacks) leaves the variant and the
        // active preset as they were and ends as Rejected.
        // param variant: The live game variant (GameGlobals + 0x14E8). Written only on success.
        // param roundState: Round state byte of the game engine globals.
        // param gameMode: Game mode byte of the game globals.
        // note: Game thread, inside the tick hook. Never throws.
        auto ApplyBetweenRounds(std::span<std::uint8_t> variant, std::uint8_t roundState, std::uint8_t gameMode) -> void;

        // Makes the next loads write no preset.
        auto ClearActive() -> void;

        // return: Whether a preset is active.
        auto HasActive() const -> bool;

        // Writes the active preset into the game variant of a start configuration and reads it back: saves the base value of every field
        // of the catalog (with the catalog), plans the preset for the load, applies it and verifies it.
        // param configuration: The whole configuration the engine starts with. It has to be a copy: the one of MCC is never written.
        // return: The outcome, also left in the store. Anything but AppliedAtLoad leaves the configuration as it was.
        // note: Called by the start hook on the MCC thread. Never throws.
        auto ApplyToStart(std::span<std::byte> configuration) -> PresetOutcome;

    private:
        Log& m_Log;
        SettingsStore& m_SettingsStore;
        PresetStore& m_PresetStore;

        // Reads the compiled preset and the catalog named, without touching the store except the result of a failure.
        auto Load(std::string_view name, std::shared_ptr<const Blam::Layout::Game::Type::VariantPreset::CompiledPreset>& preset,
            std::shared_ptr<const Blam::Layout::Game::Type::VariantPreset::PresetCatalog>& catalog) -> bool;

        // Leaves the outcome and its reason in the store and logs it.
        // A request id makes the result count only while that request is the latest one.
        auto Finish(std::optional<std::uint64_t> requestId, PresetOutcome outcome, std::string_view name, std::string message) -> PresetOutcome;
    };
}