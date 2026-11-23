module Engine.Control.Input.System;
import :AgentClientService;

import Common.Engine.Type;
import Control.Action.Type;
import Control.Action.System;
import Control.Client.Type;
import Control.Client.System;
import Engine.Control.Input.Type;
import std;

namespace
{
    using Common::Engine::Type::k_NoHandle;
    using Common::Engine::Type::k_NoInputUser;
    using Common::Engine::Type::k_NoLocalIndex;

    using ::Control::Action::System::MakeNeutralOverride;
    using ::Control::Action::Type::PossessedAction;
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

    using Engine::Control::Input::Type::AgentClient;
    using Engine::Control::Input::Type::AgentDelivery;
}

namespace Engine::Control::Input::System
{
    auto AgentClientService::StepTick(std::uintptr_t threadContext, std::size_t playerIndex, const AgentDelivery& delivery,
        const ClientSettings& settings, const std::optional<UnitClientState>& unit) -> PossessedAction
    {
        AgentClient* agent{ m_AgentClientStore.Find(playerIndex) };
        if (agent == nullptr) return PossessedAction{ .Override = MakeNeutralOverride(unit) };

        // note: The window advances on every tick, with or without a unit, so a respawn in the middle of a window is at the right tick of it.
        const std::uint32_t tickInWindow{ AdvanceAgentWindow(*agent, delivery.Sequence) };

        if (!unit)
        {
            ResetAgentUnit(*agent);
            m_AgentClientStore.CountTick(playerIndex, false, false, false);
            return PossessedAction{ .Override = MakeNeutralOverride(std::nullopt) };
        }

        // note: The state first: a new unit forgets the controls the synthesizer held, and the frame of this tick is built after that.
        const ClientState state{ PrepareAgentFrame(*agent, *unit) };
        const InputFrame frame{ MakeAgentTickFrame(*agent, delivery, tickInWindow, settings) };

        const ClientContextSource source{
            .ThreadContext = threadContext,
            .InputUser = k_NoInputUser,
            .LocalIndex = k_NoLocalIndex,
            .PlayerHandle = m_BotService.GetPlayerHandle(threadContext, playerIndex).value_or(k_NoHandle),
            .ClientZoomLevel = state.ZoomLevel,
            .DualWieldFlags = 0,
            .BoostValue = frame.Events[static_cast<std::size_t>(InputEvent::ZoomValue)].Value,
            .BoostAltValue = frame.Events[static_cast<std::size_t>(InputEvent::ZoomValueAlt)].Value };
        const UnitContext context{ m_BotService.GetClientContext(*unit, source, settings, settings.TickSeconds) };

        const ClientStep step{ StepClient(frame, state, context, settings, settings.TickSeconds) };
        if (step.Status != ClientStatus::Ran)
        {
            m_AgentClientStore.CountTick(playerIndex, false, false, false);
            return PossessedAction{ .Override = MakeNeutralOverride(unit) };
        }

        AccumulateFrame(agent->Client, step, context);

        const bool isUnsupported{ step.Output.Unsupported != 0 };
        const bool isUnknownRead{ step.Output.UnknownRead != 0 };
        m_AgentClientStore.CountTick(playerIndex, true, isUnsupported, isUnknownRead);

        if ((isUnsupported || isUnknownRead) && !std::exchange(agent->IsWarned, true))
        {
            m_Log.Message("[AgentClientService] WARNING: The client of the agent of player {} left a branch it does not port (unsupported {:#x}) or read a"
                " unit fact it does not know (unknown {:#x}).", playerIndex, step.Output.Unsupported, step.Output.UnknownRead);
        }

        return TakeTickAction(agent->Client);
    }
}