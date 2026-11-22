export module Control.Client.System;

import Control.Client.Type;
import std;

namespace
{
    using Control::Client::Type::AgentInput;
    using Control::Client::Type::AgentInputReducer;
    using Control::Client::Type::AgentInputState;
    using Control::Client::Type::ClientSettings;
    using Control::Client::Type::ClientState;
    using Control::Client::Type::ClientStep;
    using Control::Client::Type::InputFrame;
    using Control::Client::Type::UnitContext;
}

export namespace Control::Client::System
{
    // Builds what the engine client (0x5df80) leaves in its output and in its client block for one call, from the event table of the
    // input user, the state the block had, the unit it controls and the user settings (D356).
    // param frame: The table of the input user, already resolved.
    // param state: Client block state before the call.
    // param unit: What the engine says about the unit the client controls.
    // param settings: Settings of the user. Passed in, never read here (D359).
    // param dt: Seconds of the tick.
    // return: The output and the state after. Without the keyboard and mouse mode (or the mouse state) nothing runs, the state comes back
    // unchanged and the status says why. Branches that need engine state not in the context set a bit of Output.Unsupported.
    // note: Pure. Same inputs, same bytes.
    auto StepClient(const InputFrame& frame, const ClientState& state, const UnitContext& unit, const ClientSettings& settings, float dt)
        -> ClientStep;

    // Adds the look delta of a step to the absolute yaw and pitch of the state: the yaw wraps into [0, 2 pi] and the pitch stays
    // inside the limits of the unit.
    // note: Only the part of the engine aim integration (0x6375c) that moves the aim by a delta. Turn limit and seat limits are not part
    // of it.
    auto IntegrateAim(const ClientState& state, float yawDelta, float pitchDelta, const UnitContext& unit) -> ClientState;

    // Makes an AgentInput sane: move axes clamped to [-1, 1], NaN and infinity as 0 (also in the look), bits outside the controls dropped.
    auto SanitizeAgentInput(const AgentInput& input) -> AgentInput;

    // Builds the keyboard and mouse frame of tick tickInWindow of a window of windowTicks ticks for an agent (D363).
    // param input: What the agent decided, sanitized here.
    // param settings: Client settings, for the seconds of a tick.
    // param turnLimit: Radians per tick and per axis the look may ask. Zero or less means no limit. What it cuts is lost.
    // param state: Per agent, updated by this call. Call it for ticks 0 to windowTicks - 1 in order.
    // return: The frame of that tick: the events of the controls that are down (primary and secondary), the move axes and the look in the
    // raw units the client reads, so StepClient without zoom turns what was asked.
    // note: Pure and without allocation. Synthesis, StepClient and reduction give back the same AgentInput with windowTicks of 3 or more.
    // With 2 a tap of a control that was down comes back as a hold, and with 1 a press again and that tap are lost (the bridge uses 4, D304).
    auto MakeAgentFrame(const AgentInput& input, std::int32_t tickInWindow, std::int32_t windowTicks, const ClientSettings& settings,
        float turnLimit, AgentInputState& state) -> InputFrame;

    // Adds the frame of one client step to the window of the reducer (D364).
    // param lookYaw: Look the table asked in that step, before zoom and damp (ClientOutput::LookYawRequested).
    auto AddAgentFrame(AgentInputReducer& reducer, const InputFrame& frame, float lookYaw, float lookPitch) -> void;

    // Closes the window: the AgentInput of the frames added, and the reducer starts the next window.
    // return: Level is the primary event down in the last frame, Edge any frame with down count 1, axes of the last frame and the look
    // summed. A window without frames gives no move, no look, no edge and the Level of the last frame seen.
    auto CloseAgentInput(AgentInputReducer& reducer) -> AgentInput;
}