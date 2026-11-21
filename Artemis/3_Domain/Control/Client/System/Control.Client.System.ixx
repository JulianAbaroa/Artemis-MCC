export module Control.Client.System;

import Control.Client.Type;
import std;

namespace
{
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
}