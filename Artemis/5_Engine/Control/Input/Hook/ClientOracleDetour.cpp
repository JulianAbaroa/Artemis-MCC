module;

#include <windows.h>

module Engine.Control.Input.Hook;
import :ClientOracle;

import Common.Engine.Type;
import Control.Action.Type;
import Control.Client.Type;
import Control.Client.System;
import Engine.Base.Hook.System;
import Engine.Base.Memory.Type;
import Engine.Control.Input.System;
import Blam.Layout.Action.Type;
import std;

namespace
{
    namespace Signature = Engine::Base::Memory::Type::Signature;

    using ::Control::Action::Type::ActionOverride;
    using ::Control::Action::Type::UnitClientState;
    using ::Control::Client::System::StepClient;
    using ::Control::Client::Type::ClientContextSource;
    using ::Control::Client::Type::ClientSettings;
    using ::Control::Client::Type::ClientState;
    using ::Control::Client::Type::ClientStep;
    using ::Control::Client::Type::ClientStatus;
    using ::Control::Client::Type::InputFrame;
    using ::Control::Client::Type::UnitContext;

    using ::Control::Client::Type::k_NoZoom;

    using Common::Engine::Type::k_MaxLocalUsers;
    using Common::Engine::Type::k_NoHandle;

    using Engine::Base::Hook::System::k_DrainTimeout;

    using Engine::Control::Input::System::CompareClient;
    using Engine::Control::Input::System::CountClient;
    using Engine::Control::Input::System::FindInputPlayer;
    using Engine::Control::Input::System::FormatClientLine;
    using Engine::Control::Input::System::FormatClientStateFields;
    using Engine::Control::Input::System::GetEngineThreadContext;
    using Engine::Control::Input::System::LocateControlTag;
    using Engine::Control::Input::System::ReadControlTag;
    using Engine::Control::Input::System::ReadClientState;
    using Engine::Control::Input::System::ReadInputFrame;

    using Engine::Control::Input::Type::ClientDifferences;
    using Engine::Control::Input::Type::ShadowField;
    using Engine::Control::Input::Type::k_ClientBlockOffset;
    using Engine::Control::Input::Type::k_ClientBlockWindow;
    using Engine::Control::Input::Type::k_ClientInputUsers;
    using Engine::Control::Input::Type::k_ClientLogLimit;
    using Engine::Control::Input::Type::k_ClientBufferLines;
    using Engine::Control::Input::Type::k_ClientLinesPerKind;
    using Engine::Control::Input::Type::k_ClientMaxFileLines;
    using Engine::Control::Input::Type::k_ClientOracleFile;
    using Engine::Control::Input::Type::k_ClientOutputCompared;
    using Engine::Control::Input::Type::k_ClientOutputSize;
    using Engine::Control::Input::Type::k_ClockSlot;
    using Engine::Control::Input::Type::k_ClockTickSecondsOffset;
    using Engine::Control::Input::Type::k_ClockTicksPerSecondOffset;
    using Engine::Control::Input::Type::k_ControlBlockStride;
    using Engine::Control::Input::Type::k_ControlGlobalsSlot;
    using Engine::Control::Input::Type::k_ControlSettingsDefaultBit;
    using Engine::Control::Input::Type::k_DualWieldKeyboardRva;
    using Engine::Control::Input::Type::k_DualWieldPadRva;
    using Engine::Control::Input::Type::k_DualWieldPadStride;
    using Engine::Control::Input::Type::k_InputModeCount;
    using Engine::Control::Input::Type::k_InputModeRva;
    using Engine::Control::Input::Type::k_ControlSettingsDefaultRva;
    using Engine::Control::Input::Type::k_ControlSettingsRva;
    using Engine::Control::Input::Type::k_ControlSettingsStride;
    using Engine::Control::Input::Type::k_ControlTagJumpTimerOffset;
    using Engine::Control::Input::Type::k_ControlTagZoomCancelOffset;
    using Engine::Control::Input::Type::k_CrouchToggleRva;
    using Engine::Control::Input::Type::k_HoldThresholdOffset;
    using Engine::Control::Input::Type::k_InputEventTableRva;
    using Engine::Control::Input::Type::k_InputTableSize;
    using Engine::Control::Input::Type::k_MeleeHoldsUseRva;
    using Engine::Control::Input::Type::k_PlayerControlAnchorWindow;
    using Engine::Control::Input::Type::k_PlayerControlRva;
    using Engine::Control::Input::Type::k_SettingsAimSeatedOffset;
    using Engine::Control::Input::Type::k_SettingsAimZoomedOffset;
    using Engine::Control::Input::Type::k_SettingsJumpFlagOffset;

    using Blam::Layout::Action::Type::Size::k_MaxActionPlayers;

    // Milliseconds per second, to turn the hold threshold of the tag into the one the engine compares.
    constexpr float k_MillisecondsPerSecond{ 1000.0f };

    template<typename T>
    auto ReadMemory(std::uintptr_t address) -> T
    {
        T value{};
        std::memcpy(&value, reinterpret_cast<const void*>(address), sizeof(T));
        return value;
    }

    // return: Path of the oracle file inside the Artemis folder of LOCALAPPDATA, or empty if the variable is not set.
    auto GetOracleFilePath() -> std::string
    {
        std::array<char, MAX_PATH> buffer{};
        const DWORD length{ GetEnvironmentVariableA("LOCALAPPDATA", buffer.data(), static_cast<DWORD>(buffer.size())) };
        if (length == 0 || length >= buffer.size()) return {};

        return std::format("{}\\Artemis\\{}", std::string_view{ buffer.data(), length }, k_ClientOracleFile);
    }
}

namespace Engine::Control::Input::Hook
{
    auto __fastcall ClientOracleDetour::HookedPlayerControl(std::int32_t inputUser, std::uint32_t localIndex, float deltaTime, float unused,
        void* tableOut, float* output) -> void
    {
        if (s_OriginalFunction == nullptr) return;

        Engine::Base::Hook::System::InFlightScope scope(s_InFlight);

        ClientOracleDetour* self{ s_Instance.load() };

        // note: The switch is an atomic: with the check off the call costs one load and no lock.
        const bool isOn{ self != nullptr && output != nullptr && inputUser >= 0 && static_cast<std::size_t>(inputUser) < k_ClientInputUsers
            && self->m_PlayerActionStore.IsShadowEnabled() };
        if (!isOn)
        {
            s_OriginalFunction(inputUser, localIndex, deltaTime, unused, tableOut, output);
            return;
        }

        const std::uintptr_t threadContext{ GetEngineThreadContext(self->m_EngineTlsIndex) };
        const std::optional<std::size_t> player{ threadContext != 0 ? FindInputPlayer(threadContext, inputUser) : std::nullopt };
        const std::uintptr_t controlGlobals{ threadContext != 0 ? ReadMemory<std::uintptr_t>(threadContext + k_ControlGlobalsSlot) : 0 };

        bool isOverridden{ true };
        if (player)
        {
            std::array<std::optional<ActionOverride>, k_MaxActionPlayers> overrides{};
            self->m_PlayerActionStore.CopyOverrides(overrides);
            isOverridden = overrides[*player].has_value();
        }

        if (!player || isOverridden || controlGlobals == 0 || self->m_ModuleBase == 0)
        {
            s_OriginalFunction(inputUser, localIndex, deltaTime, unused, tableOut, output);
            return;
        }

        const std::uintptr_t tableAddress{ self->m_ModuleBase + k_InputEventTableRva + (static_cast<std::size_t>(inputUser) * k_InputTableSize) };
        const std::uintptr_t blockAddress{ controlGlobals + (static_cast<std::size_t>(inputUser) * k_ControlBlockStride) + k_ClientBlockOffset };

        std::array<std::uint8_t, k_InputTableSize> table{};
        std::array<std::uint8_t, k_ClientBlockWindow> blockBefore{};
        std::array<std::uint8_t, k_ClientBlockWindow> blockAfter{};
        std::array<std::uint8_t, k_ClientOutputSize> outputBefore{};
        std::array<std::uint8_t, k_ClientOutputSize> outputAfter{};

        std::memcpy(table.data(), reinterpret_cast<const void*>(tableAddress), table.size());
        std::memcpy(blockBefore.data(), reinterpret_cast<const void*>(blockAddress), blockBefore.size());
        std::memcpy(outputBefore.data(), output, outputBefore.size());

        s_OriginalFunction(inputUser, localIndex, deltaTime, unused, tableOut, output);

        std::memcpy(blockAfter.data(), reinterpret_cast<const void*>(blockAddress), blockAfter.size());
        std::memcpy(outputAfter.data(), output, outputAfter.size());

        self->Check(inputUser, localIndex, deltaTime, threadContext, *player, table, blockBefore, blockAfter, outputBefore, outputAfter);
    }

    auto ClientOracleDetour::Check(std::int32_t inputUser, std::uint32_t localIndex, float deltaTime, std::uintptr_t threadContext,
        std::size_t playerIndex, std::span<const std::uint8_t> table, std::span<const std::uint8_t> blockBefore,
        std::span<const std::uint8_t> blockAfter, std::span<const std::uint8_t> outputBefore, std::span<const std::uint8_t> outputAfter)
        -> void
    {
        const InputFrame frame{ ReadInputFrame(std::span<const std::uint8_t, k_InputTableSize>{ table }) };
        if (!frame.IsMouse)
        {
            m_PlayerActionStore.AddShadow(ShadowField::ClientNotComparable, true);
            if (!m_IsWrongModeLogged)
            {
                m_IsWrongModeLogged = true;
                m_Log.Message("[ClientOracleDetour] WARNING: The table of input user {} is not in the keyboard and mouse state: the client"
                    " oracle does not compare it.", inputUser);
            }
            return;
        }

        const std::optional<ClientSettings> settings{ this->ReadSettings(localIndex, threadContext) };
        const bool isClean{ std::ranges::all_of(outputBefore.first(k_ClientOutputCompared), [](std::uint8_t byte) { return byte == 0; }) };
        if (!settings || !isClean)
        {
            m_PlayerActionStore.AddShadow(ShadowField::ClientNotComparable, true);
            return;
        }

        const std::optional<UnitClientState> unitState{ m_BotService.GetUnitClientState(threadContext, playerIndex) };
        const ClientState state{ ReadClientState(std::span<const std::uint8_t, k_ClientBlockWindow>{ blockBefore }) };

        const ClientContextSource source{
            .ThreadContext = threadContext,
            .InputUser = inputUser,
            .LocalIndex = localIndex,
            .PlayerHandle = m_BotService.GetPlayerHandle(threadContext, playerIndex).value_or(k_NoHandle),
            .ClientZoomLevel = state.ZoomLevel,
            .DualWieldFlags = this->ReadDualWieldFlags(localIndex) };
        const UnitContext context{ unitState ? m_BotService.GetClientContext(*unitState, source, *settings, deltaTime) : UnitContext{ .Known = 0 } };

        const ClientStep step{ StepClient(frame, state, context, *settings, deltaTime) };
        if (step.Status != ClientStatus::Ran || step.Output.Unsupported != 0)
        {
            m_PlayerActionStore.AddShadow(ShadowField::ClientNotComparable, true);
            return;
        }

        const ClientDifferences differences{ CompareClient(step, std::span<const std::uint8_t, k_ClientOutputSize>{ outputAfter },
            std::span<const std::uint8_t, k_ClientBlockWindow>{ blockAfter }) };

        const bool isZoomed{ state.ZoomLevel != k_NoZoom };
        if (step.Output.UnknownRead != 0 && !m_IsPartialContextLogged)
        {
            m_IsPartialContextLogged = true;
            m_Log.Message("[ClientOracleDetour] WARNING: The unit context is partial (unknown facts {:X} read): those calls go to"
                " 'Client partial', not to the fields. First call shown only.", step.Output.UnknownRead);
        }

        this->Report(step, differences, context, isZoomed, inputUser, deltaTime, table, blockBefore);
    }

    auto ClientOracleDetour::Report(const ClientStep& step, const ClientDifferences& differences, const UnitContext& context, bool isZoomed,
        std::int32_t inputUser, float deltaTime, std::span<const std::uint8_t> table, std::span<const std::uint8_t> blockBefore) -> void
    {
        std::uint64_t firstMismatch{ 0 };
        const auto counts{ CountClient(step, differences, context, isZoomed) };
        for (std::size_t index{ 0 }; index < counts.Size; ++index)
        {
            const std::uint64_t mismatched{ m_PlayerActionStore.AddShadow(counts.Items[index].Field, counts.Items[index].IsMatch) };
            if (!counts.Items[index].IsMatch && firstMismatch == 0) firstMismatch = mismatched;
        }

        if (differences.IsEmpty()) return;

        if (firstMismatch <= k_ClientLogLimit)
        {
            m_Log.Message("[ClientOracleDetour] WARNING: Client of input user {} differs (unknown facts read {:X}): move {}, look {}, triggers {},"
                " tail {}, buttons {:X}, events {:X}, lanes {:X}, state fields{}.", inputUser, step.Output.UnknownRead, differences.IsMove,
                differences.IsLook, differences.IsTriggers, differences.IsTail, differences.Buttons, differences.Events, differences.Lanes,
                FormatClientStateFields(differences));
        }

        // note: One line per mismatch for an offline test. Only formatted here: the Main thread writes it.
        this->Enqueue(FormatClientLine(inputUser, deltaTime, differences, step.Output.UnknownRead, table, blockBefore));
    }

    auto ClientOracleDetour::Enqueue(std::string line) -> void
    {
        const std::lock_guard<std::mutex> lock{ m_BufferMutex };

        if (m_OraclePath.empty() || m_AcceptedLines >= k_ClientMaxFileLines) return;

        // note: The kind is what differs (from "unknown=" to " table="), without the dt and the bytes.
        const std::size_t kindStart{ line.find(" unknown=") };
        const std::size_t kindEnd{ line.find(" table=") };
        if (kindStart != std::string::npos && kindEnd != std::string::npos && kindEnd > kindStart)
        {
            std::uint32_t& kindLines{ m_LinesPerKind[line.substr(kindStart, kindEnd - kindStart)] };
            if (kindLines >= k_ClientLinesPerKind) return;
            ++kindLines;
        }

        if (m_BufferedLines.size() >= k_ClientBufferLines)
        {
            ++m_DroppedLines;
            return;
        }

        ++m_AcceptedLines;
        m_BufferedLines.push_back(std::move(line));
    }

    auto ClientOracleDetour::Flush() -> void
    {
        std::deque<std::string> lines{};
        std::uint64_t dropped{ 0 };
        {
            const std::lock_guard<std::mutex> lock{ m_BufferMutex };
            lines.swap(m_BufferedLines);
            dropped = std::exchange(m_DroppedLines, 0);
        }

        if (dropped != 0)
        {
            m_Log.Message("[ClientOracleDetour] WARNING: {} oracle lines dropped: the buffer was full before the Main thread wrote it.", dropped);
        }

        if (lines.empty()) return;

        std::ofstream file{ m_OraclePath, std::ios::app };
        if (!file) return;

        for (const std::string& line : lines) file << line << '\n';
    }

    auto ClientOracleDetour::ReadSettings(std::uint32_t localIndex, std::uintptr_t threadContext) const -> std::optional<ClientSettings>
    {
        if (m_ModuleBase == 0 || localIndex >= k_MaxLocalUsers) return std::nullopt;

        const std::uintptr_t tag{ ReadControlTag(m_ControlTag) };
        if (tag == 0) return std::nullopt;

        const std::uintptr_t clock{ ReadMemory<std::uintptr_t>(threadContext + k_ClockSlot) };
        if (clock == 0) return std::nullopt;

        std::uintptr_t block{ m_ModuleBase + k_ControlSettingsRva + (localIndex * k_ControlSettingsStride) };
        if (((ReadMemory<std::uint32_t>(block) >> k_ControlSettingsDefaultBit) & 1u) != 0) block = m_ModuleBase + k_ControlSettingsDefaultRva;

        ClientSettings settings{};
        settings.HoldThresholdMs = static_cast<std::int32_t>(ReadMemory<float>(tag + k_HoldThresholdOffset) * k_MillisecondsPerSecond);
        settings.ZoomCancelThreshold = ReadMemory<float>(tag + k_ControlTagZoomCancelOffset);
        settings.JumpTimerSeconds = ReadMemory<float>(tag + k_ControlTagJumpTimerOffset);
        settings.AimScaleZoomed = ReadMemory<float>(block + k_SettingsAimZoomedOffset);
        settings.AimScaleSeated = ReadMemory<float>(block + k_SettingsAimSeatedOffset);
        settings.IsJumpFlag = ReadMemory<std::uint8_t>(block + k_SettingsJumpFlagOffset) != 0;
        settings.IsCrouchToggle = ReadMemory<std::uint8_t>(m_ModuleBase + k_CrouchToggleRva) != 0;
        settings.IsMeleeHoldsUse = ReadMemory<std::uint8_t>(m_ModuleBase + k_MeleeHoldsUseRva) != 0;
        settings.TicksPerSecond = ReadMemory<std::int16_t>(clock + k_ClockTicksPerSecondOffset);
        settings.TickSeconds = ReadMemory<float>(clock + k_ClockTickSecondsOffset);
        return settings;
    }

    auto ClientOracleDetour::ReadDualWieldFlags(std::uint32_t localIndex) const -> std::optional<std::uint8_t>
    {
        if (m_ModuleBase == 0) return std::nullopt;

        // note: A local index past the array reads the mode of the first user, like the engine does.
        const std::uint32_t modeIndex{ localIndex < k_InputModeCount ? localIndex : 0 };
        const auto mode{ ReadMemory<std::int32_t>(m_ModuleBase + k_InputModeRva + (modeIndex * sizeof(std::int32_t))) };
        if (mode != 0) return ReadMemory<std::uint8_t>(m_ModuleBase + k_DualWieldKeyboardRva);

        return ReadMemory<std::uint8_t>(m_ModuleBase + k_DualWieldPadRva + (static_cast<std::uintptr_t>(localIndex) * k_DualWieldPadStride));
    }

    auto ClientOracleDetour::Install() -> bool
    {
        if (m_IsHookInstalled.load()) return true;

        // note: The anchor is a place inside the function (the control tag load), so no bytes of the prologue are needed: the rva and the
        // unwind table of the module say where the function begins. The same match gives the control tag, with no second scan.
        std::uintptr_t anchor{ 0 };
        const std::uintptr_t function{ m_AOBService.FindFunctionAtRva(Signature::k_PlayerControlTagLoad, k_PlayerControlRva,
            k_PlayerControlAnchorWindow, L"haloreach.dll", &anchor) };
        if (function == 0)
        {
            m_Log.Message("[ClientOracleDetour] ERROR: Failed to find the player control (rva {:X}). The client oracle stays off.",
                k_PlayerControlRva);
            return false;
        }

        m_ModuleBase = function - k_PlayerControlRva;
        m_ControlTag = LocateControlTag(anchor);
        m_EngineTlsIndex = Engine::Base::Hook::System::ResolveTlsIndex(m_AOBService.FindPattern(Signature::k_TelemetryIdModifier));
        if (!m_ControlTag.IsValid() || !m_EngineTlsIndex)
        {
            m_Log.Message("[ClientOracleDetour] ERROR: Failed to find the control tag or the thread context. The client oracle stays off.");
            return false;
        }

        {
            const std::lock_guard<std::mutex> lock{ m_BufferMutex };
            m_OraclePath = GetOracleFilePath();
            m_BufferedLines.clear();
            m_DroppedLines = 0;
            m_AcceptedLines = 0;
            m_LinesPerKind.clear();
        }

        void* functionAddress{ reinterpret_cast<void*>(function) };
        m_FunctionAddress.store(functionAddress);

        if (!Engine::Base::Hook::System::InstallDetour(functionAddress, reinterpret_cast<void*>(&HookedPlayerControl),
            reinterpret_cast<void**>(&s_OriginalFunction), "[ClientOracleDetour]", m_Log))
        {
            m_FunctionAddress.store(nullptr);
            s_Instance.store(nullptr);
            return false;
        }

        // note: Set after everything was validated and the hook is in: a call that sees null just runs the original.
        s_Instance.store(this);
        m_IsHookInstalled.store(true);
        return true;
    }

    auto ClientOracleDetour::Uninstall() -> void
    {
        if (!m_IsHookInstalled.load()) return;

        s_Instance.store(nullptr);
        Engine::Base::Hook::System::DisableDetour(m_FunctionAddress.load());
        Engine::Base::Hook::System::WaitForDrain(s_InFlight, k_DrainTimeout);
        Engine::Base::Hook::System::RemoveDetour(m_FunctionAddress.load(), "[ClientOracleDetour]", m_Log);

        m_IsHookInstalled.store(false);

        // note: The hook no longer runs: what it buffered goes to the file now, so a session shorter than the Main loop period loses nothing.
        this->Flush();
    }
}