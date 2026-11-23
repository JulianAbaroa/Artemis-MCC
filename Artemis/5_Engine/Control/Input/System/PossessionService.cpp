module Engine.Control.Input.System;
import :PossessionService;

import Common.Engine.Type;
import Control.Action.Type;
import Control.Client.Type;
import Control.Client.System;
import Engine.Control.Input.Type;
import std;

namespace
{
    using Common::Engine::Type::k_NoHandle;

    using ::Control::Action::Type::UnitClientState;
    using ::Control::Client::System::StepClient;
    using ::Control::Client::Type::ClientContextSource;
    using ::Control::Client::Type::ClientSettings;
    using ::Control::Client::Type::ClientState;
    using ::Control::Client::Type::ClientStatus;
    using ::Control::Client::Type::ClientStep;
    using ::Control::Client::Type::InputEvent;
    using ::Control::Client::Type::InputFrame;
    using ::Control::Client::Type::UnitContext;

    using Engine::Control::Input::Type::ControlTagLocator;
    using Engine::Control::Input::Type::ClientDriven;
    using Engine::Control::Input::Type::k_InputTableSize;
}

namespace Engine::Control::Input::System
{
    auto PossessionService::IsActive() const -> bool
    {
        return m_PossessionStore.IsActive();
    }

    auto PossessionService::StepFrame(std::uintptr_t moduleBase, const ControlTagLocator& controlTag, std::uintptr_t threadContext,
        std::int32_t inputUser, std::uint32_t localIndex, float deltaTime, const std::array<std::uint8_t, k_InputTableSize>& table) -> void
    {
        if (m_PossessionStore.IsDisabled()) return;

        if (!m_PossessionStore.IsTickThread())
        {
            if (m_PossessionStore.Disable())
            {
                m_Log.Message("[PossessionService] ERROR: The frames of the player control run on thread {} and the ticks on thread {}. The possession shares"
                    " its state without a lock, so it is turned off.", std::this_thread::get_id(), m_PossessionStore.GetTickThread());
            }
            return;
        }

        ClientDriven* bot{ m_PossessionStore.Find() };
        if (bot == nullptr) return;

        if (m_PossessionStore.MarkThreadsLogged())
        {
            m_Log.Message("[PossessionService] INFO: Possession started: frames on thread {}, ticks on thread {}.", std::this_thread::get_id(),
                m_PossessionStore.GetTickThread());
        }

        const std::optional<UnitClientState> unit{ m_BotService.GetUnitClientState(threadContext, bot->Player) };
        if (!unit) return;

        const std::optional<ClientSettings> settings{ ReadClientSettings(moduleBase, ReadControlTag(controlTag), threadContext, localIndex) };
        if (!settings) return;

        const InputFrame frame{ ReadInputFrame(std::span<const std::uint8_t, k_InputTableSize>{ table }) };
        if (!frame.IsMouse) return;

        const ClientState state{ PrepareFrame(*bot, *unit) };

        // note: User level facts (accessibility, look offset, alternate control) are the ones of the player of the input user, the unit is the bot's.
        const std::optional<std::size_t> human{ FindInputPlayer(threadContext, inputUser) };
        const ClientContextSource source{
            .ThreadContext = threadContext,
            .InputUser = inputUser,
            .LocalIndex = localIndex,
            .PlayerHandle = human ? m_BotService.GetPlayerHandle(threadContext, *human).value_or(k_NoHandle) : k_NoHandle,
            .ClientZoomLevel = state.ZoomLevel,
            .DualWieldFlags = ReadDualWieldRestrictions(moduleBase, localIndex),
            .BoostValue = frame.Events[static_cast<std::size_t>(InputEvent::ZoomValue)].Value,
            .BoostAltValue = frame.Events[static_cast<std::size_t>(InputEvent::ZoomValueAlt)].Value };
        const UnitContext context{ m_BotService.GetClientContext(*unit, source, *settings, deltaTime) };

        const ClientStep step{ StepClient(frame, state, context, *settings, deltaTime) };
        if (step.Status != ClientStatus::Ran) return;

        AccumulateFrame(*bot, step, context);
    }
}