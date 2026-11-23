export module Engine.Control.Input.System:ClientSource;

import Common.Engine.Type;
import Control.Client.Type;
import Engine.Control.Input.Type;
import std;

namespace
{
    using Control::Client::Type::ClientSettings;

    using Common::Engine::Type::k_MaxLocalUsers;

    using Control::Client::Type::k_MillisecondsPerSecond;

    using Engine::Control::Input::Type::ControlTagLocator;
    using Engine::Control::Input::Type::k_ClockSlot;
    using Engine::Control::Input::Type::k_ClockTickSecondsOffset;
    using Engine::Control::Input::Type::k_ClockTicksPerSecondOffset;
    using Engine::Control::Input::Type::k_ControlSettingsDefaultBit;
    using Engine::Control::Input::Type::k_ControlSettingsDefaultRva;
    using Engine::Control::Input::Type::k_ControlSettingsRva;
    using Engine::Control::Input::Type::k_ControlSettingsStride;
    using Engine::Control::Input::Type::k_ControlTagJumpTimerOffset;
    using Engine::Control::Input::Type::k_ControlTagZoomCancelOffset;
    using Engine::Control::Input::Type::k_CrouchToggleRva;
    using Engine::Control::Input::Type::k_DualWieldKeyboardRva;
    using Engine::Control::Input::Type::k_DualWieldPadRva;
    using Engine::Control::Input::Type::k_DualWieldPadStride;
    using Engine::Control::Input::Type::k_HoldThresholdOffset;
    using Engine::Control::Input::Type::k_InputEventTableRva;
    using Engine::Control::Input::Type::k_InputModeCount;
    using Engine::Control::Input::Type::k_InputModeRva;
    using Engine::Control::Input::Type::k_InputTableSize;
    using Engine::Control::Input::Type::k_MeleeHoldsUseRva;
    using Engine::Control::Input::Type::k_SettingsAimSeatedOffset;
    using Engine::Control::Input::Type::k_SettingsAimZoomedOffset;
    using Engine::Control::Input::Type::k_SettingsJumpFlagOffset;

    template<typename T>
    auto ReadRaw(std::uintptr_t address) -> T
    {
        T value{};
        std::memcpy(&value, reinterpret_cast<const void*>(address), sizeof(T));
        return value;
    }

    // Fills the settings that belong to the game and not to a user: the thresholds of the control tag and the clock of the engine.
    // return: False when the tag is not loaded or the clock is missing.
    inline auto ReadGameSettings(std::uintptr_t tag, std::uintptr_t threadContext, ClientSettings& settings) -> bool
    {
        if (tag == 0) return false;

        const std::uintptr_t clock{ ReadRaw<std::uintptr_t>(threadContext + k_ClockSlot) };
        if (clock == 0) return false;

        settings.HoldThresholdMs = static_cast<std::int32_t>(ReadRaw<float>(tag + k_HoldThresholdOffset) * static_cast<float>(k_MillisecondsPerSecond));
        settings.ZoomCancelThreshold = ReadRaw<float>(tag + k_ControlTagZoomCancelOffset);
        settings.JumpTimerSeconds = ReadRaw<float>(tag + k_ControlTagJumpTimerOffset);
        settings.TicksPerSecond = ReadRaw<std::int16_t>(clock + k_ClockTicksPerSecondOffset);
        settings.TickSeconds = ReadRaw<float>(clock + k_ClockTickSecondsOffset);
        return true;
    }
}

// note: What the client of the engine (0x5df80) reads about a user, in one place. The oracle and the possession both call it.
export namespace Engine::Control::Input::System
{
    // return: The settings the client of a local user uses, or nothing when the module base, the user, the control tag or the clock are missing.
    // param tag: The data of the control tag, already resolved (0 when not loaded).
    // param threadContext: Thread context of the engine, for the clock.
    // note: Game thread only.
    inline auto ReadClientSettings(std::uintptr_t moduleBase, std::uintptr_t tag, std::uintptr_t threadContext, std::uint32_t localIndex)
        -> std::optional<ClientSettings>
    {
        if (moduleBase == 0 || localIndex >= k_MaxLocalUsers) return std::nullopt;

        ClientSettings settings{};
        if (!ReadGameSettings(tag, threadContext, settings)) return std::nullopt;

        std::uintptr_t block{ moduleBase + k_ControlSettingsRva + (localIndex * k_ControlSettingsStride) };
        if (((ReadRaw<std::uint32_t>(block) >> k_ControlSettingsDefaultBit) & 1u) != 0) block = moduleBase + k_ControlSettingsDefaultRva;

        settings.AimScaleZoomed = ReadRaw<float>(block + k_SettingsAimZoomedOffset);
        settings.AimScaleSeated = ReadRaw<float>(block + k_SettingsAimSeatedOffset);
        settings.IsJumpFlag = ReadRaw<std::uint8_t>(block + k_SettingsJumpFlagOffset) != 0;
        settings.IsCrouchToggle = ReadRaw<std::uint8_t>(moduleBase + k_CrouchToggleRva) != 0;
        settings.IsMeleeHoldsUse = ReadRaw<std::uint8_t>(moduleBase + k_MeleeHoldsUseRva) != 0;
        return settings;
    }

    // return: The settings the client of Artemis uses for an agent (VW.9c.3 C, D359), or nothing when the tag or the clock are missing. What belongs to
    // the game (hold threshold, zoom cancel, jump timer, ticks per second) is read like for a user. Every preference of a user is fixed here and is never
    // copied from the user: the aim scales are 1 (no extra scale with zoom or in a seat), crouch is a hold, the jump is not a flag, melee does not hold
    // use. The game system, the session and the input are ready, and the context is not a menu.
    // param tag: The data of the control tag, already resolved (0 when not loaded).
    // param threadContext: Thread context of the engine, for the clock.
    // note: Game thread only.
    inline auto ReadAgentClientSettings(std::uintptr_t tag, std::uintptr_t threadContext) -> std::optional<ClientSettings>
    {
        ClientSettings settings{};
        if (!ReadGameSettings(tag, threadContext, settings)) return std::nullopt;

        settings.AimScaleZoomed = 1.0f;
        settings.AimScaleSeated = 1.0f;
        settings.IsCrouchToggle = false;
        settings.IsJumpFlag = false;
        settings.IsMeleeHoldsUse = false;
        settings.IsGameSystemPresent = true;
        settings.IsSessionReady = true;
        settings.IsMenuContext = false;
        settings.IsInputEnabled = true;
        return settings;
    }

    // return: The restrictions byte of dual wield for a local user, or nothing without the module base.
    inline auto ReadDualWieldRestrictions(std::uintptr_t moduleBase, std::uint32_t localIndex) -> std::optional<std::uint8_t>
    {
        if (moduleBase == 0) return std::nullopt;

        // note: A local index past the array reads the mode of the first user, like the engine does.
        const std::uint32_t modeIndex{ localIndex < k_InputModeCount ? localIndex : 0 };
        const auto mode{ ReadRaw<std::int32_t>(moduleBase + k_InputModeRva + (modeIndex * sizeof(std::int32_t))) };
        if (mode != 0) return ReadRaw<std::uint8_t>(moduleBase + k_DualWieldKeyboardRva);

        return ReadRaw<std::uint8_t>(moduleBase + k_DualWieldPadRva + (static_cast<std::uintptr_t>(localIndex) * k_DualWieldPadStride));
    }

    // return: A copy of the event table of an input user, as the engine has it this tick.
    // note: Game thread only. The caller checked the module base and the input user are valid.
    inline auto ReadInputTable(std::uintptr_t moduleBase, std::int32_t inputUser) -> std::array<std::uint8_t, k_InputTableSize>
    {
        std::array<std::uint8_t, k_InputTableSize> table{};
        std::memcpy(table.data(), reinterpret_cast<const void*>(moduleBase + k_InputEventTableRva + (static_cast<std::size_t>(inputUser) * k_InputTableSize)),
            table.size());
        return table;
    }
}