module Engine.Control.Input.System;
import :AgentClientService;

import Common.Engine.Type;
import Control.Action.Type;
import Control.Action.System;
import Control.Client.Type;
import Control.Client.System;
import Engine.Control.Input.Type;
import Blam.Layout.Action.Type;
import std;

namespace
{
    using Common::Engine::Type::k_NoInputUser;
    using Common::Engine::Type::k_NoLocalIndex;

    using Blam::Layout::Action::Type::Size::k_MaxActionPlayers;

    using ::Control::Action::System::MakeNeutralOverride;
    using ::Control::Action::Type::ClientAction;
    using ::Control::Action::Type::UnitClientState;
    using ::Control::Client::System::StepClient;
    using ::Control::Client::Type::AgentInputState;
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
    using Engine::Control::Input::Type::AgentPlan;
}

namespace Engine::Control::Input::System
{
    auto AgentClientService::PlayTick(std::uintptr_t threadContext, std::size_t playerIndex, std::optional<std::size_t> possessed,
        const std::optional<AgentDelivery>& delivery, const std::optional<ClientSettings>& settings) -> std::optional<ClientAction>
    {
        if (!delivery)
        {
            m_AgentClientStore.End(playerIndex);
            return std::nullopt;
        }

        const std::optional<std::uint32_t> handle{ m_BotService.GetPlayerHandle(threadContext, playerIndex) };
        switch (PlanAgent(*delivery, playerIndex, possessed, handle))
        {
        case AgentPlan::Stale:
            m_AgentClientStore.End(playerIndex);
            m_PlayerActionStore.ClearAgentInput(playerIndex, delivery->Handle);
            return std::nullopt;
        case AgentPlan::Suspend:
            m_AgentClientStore.Suspend(playerIndex);
            return std::nullopt;
        case AgentPlan::Run:
            break;
        }

        m_AgentClientStore.Begin(playerIndex, *handle);
        AgentClient* agent{ m_AgentClientStore.Find(playerIndex) };
        if (agent == nullptr) return std::nullopt;

        const std::optional<UnitClientState> unit{ m_BotService.GetUnitClientState(threadContext, playerIndex) };

        // note: The window advances on every tick, with or without settings or unit, so a respawn in the middle of a window is at the right tick of it.
        const std::uint32_t tickInWindow{ AdvanceAgentWindow(*agent, delivery->Sequence) };

        if (!settings)
        {
            m_AgentClientStore.CountTick(playerIndex, false, false, false);
            return ClientAction{ .Override = MakeNeutralOverride(unit) };
        }

        if (!unit)
        {
            ResetAgentUnit(*agent);
            m_AgentClientStore.CountTick(playerIndex, false, false, false);
            return ClientAction{ .Override = MakeNeutralOverride(std::nullopt) };
        }

        return this->StepUnit(threadContext, playerIndex, *handle, *agent, *delivery, tickInWindow, *settings, *unit);
    }

    auto AgentClientService::SuspendAll() -> void
    {
        for (std::size_t player{ 0 }; player < k_MaxActionPlayers; ++player) m_AgentClientStore.Suspend(player);
    }

    auto AgentClientService::StepUnit(std::uintptr_t threadContext, std::size_t playerIndex, std::uint32_t playerHandle, AgentClient& agent,
        const AgentDelivery& delivery, std::uint32_t tickInWindow, const ClientSettings& settings, const UnitClientState& unit) -> ClientAction
    {
        // note: The state first: a new unit forgets the controls the synthesizer held, and the frame of this tick is built after that.
        const ClientState state{ PrepareAgentFrame(agent, unit) };
        const AgentInputState synthesizer{ agent.Input };
        const InputFrame frame{ MakeAgentTickFrame(agent, delivery, tickInWindow, settings) };

        const ClientContextSource source{
            .ThreadContext = threadContext,
            .InputUser = k_NoInputUser,
            .LocalIndex = k_NoLocalIndex,
            .PlayerHandle = playerHandle,
            .ClientZoomLevel = state.ZoomLevel,
            .DualWieldFlags = 0,
            .BoostValue = frame.Events[static_cast<std::size_t>(InputEvent::ZoomValue)].Value,
            .BoostAltValue = frame.Events[static_cast<std::size_t>(InputEvent::ZoomValueAlt)].Value };
        const UnitContext context{ m_BotService.GetClientContext(unit, source, settings, settings.TickSeconds) };

        const ClientStep step{ StepClient(frame, state, context, settings, settings.TickSeconds) };
        if (step.Status != ClientStatus::Ran)
        {
            UndoAgentFrame(agent, synthesizer);
            m_AgentClientStore.CountTick(playerIndex, false, false, false);
            return ClientAction{ .Override = MakeNeutralOverride(unit) };
        }

        AccumulateFrame(agent.Client, step, context);

        const bool isUnsupported{ step.Output.Unsupported != 0 };
        const bool isUnknownRead{ step.Output.UnknownRead != 0 };
        m_AgentClientStore.CountTick(playerIndex, true, isUnsupported, isUnknownRead);

        if ((isUnsupported || isUnknownRead) && !std::exchange(agent.IsWarned, true))
        {
            m_Log.Message("[AgentClientService] WARNING: The client of the agent of player {} left a branch it does not port (unsupported {:#x}) or read a"
                " unit fact it does not know (unknown {:#x}).", playerIndex, step.Output.Unsupported, step.Output.UnknownRead);
        }

        return TakeTickAction(agent.Client);
    }
}