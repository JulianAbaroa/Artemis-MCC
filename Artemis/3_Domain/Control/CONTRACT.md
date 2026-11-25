# Artemis.Control

Group `3_Domain` - pure static library - the logic of the player action and of the bridge's lockstep, without touching the game.

## Responsibility
- Action: from the decision of an agent to the fields Artemis writes into the 0xA8-byte action of a tick (buttons, hold lanes with
  the engine's threshold, the facing straight to the one asked, weapon/grenade/zoom requests of a client).
- Client: the engine client of a local player (0x5df80, keyboard and mouse) as a pure function (VW.9c.1, D356): from the event table of an
  input user, the state of its client block, the context of the unit and the settings of the user to the output the engine builds
  (move axes, triggers, look delta, buttons, event bits and hold lanes) and the new state (crouch toggle, jump timer, look rates, latches).
- Lockstep: the policy of the bridge: which control is in effect for what the client asked, when a decision is due, how long to
  wait, when to suspend after timeouts, how the `BridgeAgentInput` of a bridge action becomes a sane `AgentInput` (and the way back for the imitation block), and the clock of decision ticks.

## Does not
- Write memory or call engine functions: `Engine.Control` applies the action.
- Transport anything: `Bridge.Transport` calls the lockstep policy.
- Answer questions only the engine knows (next weapon, next grenade, next zoom level, whether the unit can zoom): it asks the `UnitRules`
  port, which `Engine.Control` implements.
- Keep the grenade request of a client between ticks (D355): `PlayerActionHistory::PendingGrenade` is sent every tick until the unit selects that type, and with the
  applied type empty the next one is asked without a press, as 0x61a38 does for a local player. `UnitClientState` carries the selected type (0x37C) and the counts
  (0x37E, 0x37F) for it. The dual wield test is not ported (Reach multiplayer has none).

## Input and output
- Input: an `ActionOverride` (from the bench) or an `AgentInput` (from the bridge or the bench), the client state of the unit, the previous `PlayerActionHistory`, the control
  requests of the client.
- Output: the action fields to write and the new history; the control in effect, timeouts and the due decision.

## Public interface
Other projects may import only these modules; everything else in the project is internal.

| Module | What it gives |
|---|---|
| `Control.Action.Type` | `ActionOverride` (also the events word, the fire, zoom and extra triggers of a possessed bot), `ClientRequests` (with `operator|`, the requests added up), `ClientAction` (what the client gives a player, a possessed bot or an agent), `UnitClientState`, `PlayerActionHistory`. |
| `Control.Action.System` | `BuildAction` (writes the events word +0x34 and the triggers +0x24, +0x38, +0x3C only when not zero), `MakeClientAction` (client output to override and requests, VW.9c.2 B), `MakeNeutralOverride`, `GetHoldThresholdTicks` and the `UnitRules` port (also `GetClientContext`: the unit facts the client asks, VW.9c.1). |
| `Control.Client.Type` | `InputFrame` (the table of one input user, already resolved), `ClientState` (the client block fields kept between calls, with their offsets), `UnitContext` (what the client asks of the unit), `ClientSettings` (passed in, D359), `ClientOutput`, `ClientStep`, `ClientStatus`, `Unsupported` and the event, button and event bit constants. Agent input (VW.9c.3 B, D362-D364): `AgentControl` (13 bits), `k_AgentControlCount`, `k_AgentControlMask`, `AgentInput` (move axes, look in radians without zoom, `Level` and `Edge` bits), `k_AgentControlEvents` (the primary and secondary events each control raises, A17 sec. 9), `AgentInputState` (synthesizer state per agent) and `AgentInputReducer` (window reducer state) and `AgentInputLine` (an AgentInput held for a range of ticks with its window and turn limit). `ClientOutput` also carries `LookYawRequested` and `LookPitchRequested` (the look asked, before zoom and aim damp). `k_MillisecondsPerSecond`. |
| `Control.Client.System` | `StepClient` (one call of 0x5df80) and `IntegrateAim` (adds the look delta to yaw and pitch with the pitch limit). `SanitizeAgentInput`, `MakeAgentFrame` (AgentInput to the keyboard and mouse frame of tick k of a window, with the turn limit as a parameter) and `AddAgentFrame` / `CloseAgentInput` (frames of a window to one AgentInput), `FindAgentControl` (a control by its lowercase name, `k_AgentControlNames` in the Type module), `ParseAgentInputWords` (the text form of an AgentInput: words `key=value` with move, look, level, edge and turn, the one the test bench reads, VW.9c.3 C), `ParseAgentInputLine` (the same with the first tick, the last tick and the window before the words, `AgentInputLine`) and `AreTickRangesOverlapping` (VW.9c.3 D). |
| `Control.Lockstep.Type` | Limits and defaults of the lockstep (decision period N, latency d, timeouts) and `BridgeControlState`. |
| `Control.Lockstep.System` | `DecideControl`, `ApplyConnection`, `ToAgentInput` (the `BridgeAgentInput` the client wrote to a sane `AgentInput`, with the flag the bridge counts as a rejected action: the one place that sanitizes it, VW.9c.3 D, D365), `ToBridgeAgentInput` (the way back, for the imitation block), `IsImitationWindowEnd` (where a window of d ticks of the possessed player ends, counted from the decision tick, D364), `GetDueDecision`, `GetNeedDecision` (the decision the game thread reaches at its tick plus d, with or without the answer, once per decision: the rule of the Need marks of the timeline, P.3 C, D373), `RecordWait`, `ResumeIfAnswered`, `GetApplyTiming`, `ReadClock`, `IsDecisionTick`, `IsSparse` (Q101: the client asked for sparse, is connected and nobody needs every tick, pure). |

## Depends on
`Common`, `Blam.Layout`, `Contract`.

## State between ticks
None of its own: the caller keeps the `PlayerActionHistory` of each player and the bridge control state, and passes them in.

## Threads
Called on the game thread (the PlayerAction hook builds the action) and on the bridge thread (control). Pure functions: they never
block or allocate on the hot path.

## Errors
`StepClient` with a mode other than keyboard and mouse (or a table without the mouse state) does not run: the state comes back unchanged and
`ClientStatus` says why. A branch of 0x5df80 that needs engine state the context does not carry (flinch, frame of reference,
selection list, ability button, menu look scale) is not guessed: `Output.Unsupported` gets its bit and the step is not comparable.
`ClientContextSource` (VW.9c.2) carries what the engine side needs to resolve the facts: the thread context, the input user and local index, the
player handle of the user, the zoom level of the client block and the dual wield restrictions byte (the ticks per second come from `ClientSettings` and the dt from the call, passed to `GetClientContext` next to the source). The
kind 1 ability only goes through the engine (`Unsupported::AbilityButton`) when its tag asks for it (`Ability.NeedsEngine`) and the setting
`IsJumpFlag` is on.
The raw down byte of an event (`EventSample::DownCount`, 1 on the first frame) decides the zoom press, the two zoom steps, the crouch toggle and the
ability lane (kind 0): the engine compares it with 1 (0x5d31c), so they act on the first frame of a press and not on every frame it stays down.
The held bits (zoom held, lanes) use any non zero byte.
The boost (VW.9c.2 C): a unit whose tag (or parent tag, seat flag bit 2) has the trigger curve bit takes `Trigger4` = the clamp to [0, 1] of
`UnitContext::TriggerCurveValue`, which the engine side already ran through the tag function for the raw value of this call, and, with
`IsTriggerCurveForward` and a value above 0, the forward axis is pushed (below 0 it grows by 1, else it is 1), after the move penalty. A value
the engine side could not resolve is outside `Known` (`TriggerCurveValue`, `BoostAltEvent`, `TriggerCurveForward`), so the step says it read an
unknown. `ClientContextSource` carries the raw boost value and the raw value of the alternate event 0x26: the builder picks one by the control
profile of the player (`IsBoostAltEvent`). `Unsupported::TriggerCurve` no longer exists.
The zoom of a possessed or agent unit (VW.9c.2 C, 0x61a38): the zoom press goes to the next level of the weapon and back to none after the last
(`ClientRequests::IsZoom`), the zoom in step to the larger of the current and the next level (the last level stays, `IsZoomIn`), the zoom out step
to one level down and never below none (`IsZoomOut`). Both steps need `UnitRules::CanZoom` and ask nothing without it. A weapon switch pending wins over
all of them. A weapon switch of a seated unit raises the button 44 (`k_ActionSeatWeaponSwitchButton`) for the tick and asks no slot (the Banshee
cannons and bomb). Pending: the slot change of a seat with the unit weapon -1 and the seat flag bit 5, and the button 21 (toggle +0x1E3).
A fact of the unit the context does not know (`UnitContext::Known` lacks its `UnitFact` bit) is read as its default, and the step says so:
`Output.UnknownRead` holds the bits of the unknown facts the branches that ran actually read, and a step with a bit set is not comparable.
Not ported on purpose: the controller path (+0x6F8 = 0), the aim assist (D109), the interaction records (D118), the seat turn limits and aim
follow of 0x6375c and the pause/lock side effects on the table.
Out-of-range requests are clamped (N, d) or sanitized (`ToAgentInput` over `SanitizeAgentInput`: axes cut to [-1, 1], a value that is not finite is 0, bits 13 to 31 of `Level` and `Edge` dropped); nothing throws. The turn limit of the bridge is not in the action: it travels in the control block and the `PlayerActionStore` keeps it sane.

## Requirements
- Local players with an override and bots get exactly the same action.
- Turn limit 0 aims straight; a limit is never exceeded in one tick.
- Pure: no process, no wiring, no clock (checked by `Tools/Tests/test_components.py`).

## Tests
`ActionTests` and `LockstepTests` in `Artemis.Tests`; in game, `Agent/scripts/bridge_check.py run` and `turn`.
