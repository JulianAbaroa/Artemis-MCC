# Artemis.Control

Group `3_Domain` - pure static library - the logic of the player action and of the bridge's lockstep, without touching the game.

## Responsibility
- Action: from the decision of an agent to the fields Artemis writes into the 0xA8-byte action of a tick (buttons, hold lanes with
  the engine's threshold, limited turn of the aim, weapon/grenade/zoom requests of a client).
- Client: the engine client of a local player (0x5df80, keyboard and mouse) as a pure function (VW.9c.1, D356): from the event table of an
  input user, the state of its client block, the context of the unit and the settings of the user to the output the engine builds
  (move axes, triggers, look delta, buttons, event bits and hold lanes) and the new state (crouch toggle, jump timer, look rates, latches).
- Lockstep: the policy of the bridge: which control is in effect for what the client asked, when a decision is due, how long to
  wait, when to suspend after timeouts, how a bridge action becomes a sanitized override, and the clock of decision ticks.

## Does not
- Write memory or call engine functions: `Engine.Control` applies the action.
- Transport anything: `Bridge.Transport` calls the lockstep policy.
- Answer questions only the engine knows (next weapon, next grenade, next zoom level): it asks the `UnitRules` port, which
  `Engine.Control` implements.

## Input and output
- Input: an `ActionOverride` (from the bridge), the client state of the unit, the previous `PlayerActionHistory`, the control
  requests of the client.
- Output: the action fields to write and the new history; the control in effect, timeouts and the due decision.

## Public interface
Other projects may import only these modules; everything else in the project is internal.

| Module | What it gives |
|---|---|
| `Control.Action.Type` | `ActionOverride`, `AppliedFacing`, `ClientRequests`, `UnitClientState`, `PlayerActionHistory`. |
| `Control.Action.System` | `BuildAction`, `GetHoldThresholdTicks` and the `UnitRules` port (also `GetClientContext`: the unit facts the client asks, VW.9c.1). |
| `Control.Client.Type` | `InputFrame` (the table of one input user, already resolved), `ClientState` (the client block fields kept between calls, with their offsets), `UnitContext` (what the client asks of the unit), `ClientSettings` (passed in, D359), `ClientOutput`, `ClientStep`, `ClientStatus`, `Unsupported` and the event, button and event bit constants. |
| `Control.Client.System` | `StepClient` (one call of 0x5df80) and `IntegrateAim` (adds the look delta to yaw and pitch with the pitch limit). |
| `Control.Lockstep.Type` | Limits and defaults of the lockstep (decision period N, latency d, timeouts) and `BridgeControlState`. |
| `Control.Lockstep.System` | `DecideControl`, `ApplyConnection`, `BuildOverride`, `GetDueDecision`, `RecordWait`, `ResumeIfAnswered`, `GetApplyTiming`, `GetReleasedOverride`, `ReadClock`, `IsDecisionTick`, `IsSparse` (Q101: the client asked for sparse, is connected and nobody needs every tick, pure). |

## Depends on
`Common`, `Blam.Layout`, `Contract`.

## State between ticks
None of its own: the caller keeps the `PlayerActionHistory` of each player and the bridge control state, and passes them in.

## Threads
Called on the game thread (the PlayerAction hook builds the action) and on the bridge thread (control). Pure functions: they never
block or allocate on the hot path.

## Errors
`StepClient` with a mode other than keyboard and mouse (or a table without the mouse state) does not run: the state comes back unchanged and
`ClientStatus` says why. A branch of 0x5df80 that needs engine state the context does not carry (flinch, trigger curve, frame of reference,
selection list, ability button, menu look scale) is not guessed: `Output.Unsupported` gets its bit and the step is not comparable.
`ClientContextSource` (VW.9c.2) carries what the engine side needs to resolve the facts: the thread context, the input user and local index, the
player handle of the user, the zoom level of the client block and the dual wield restrictions byte (the ticks per second come from `ClientSettings` and the dt from the call, passed to `GetClientContext` next to the source). The
kind 1 ability only goes through the engine (`Unsupported::AbilityButton`) when its tag asks for it (`Ability.NeedsEngine`) and the setting
`IsJumpFlag` is on.
A fact of the unit the context does not know (`UnitContext::Known` lacks its `UnitFact` bit) is read as its default, and the step says so:
`Output.UnknownRead` holds the bits of the unknown facts the branches that ran actually read, and a step with a bit set is not comparable.
Not ported on purpose: the controller path (+0x6F8 = 0), the aim assist (D109), the interaction records (D118), the seat turn limits and aim
follow of 0x6375c and the pause/lock side effects on the table.
Out-of-range requests are clamped (N, d) or sanitized (`BuildOverride`); nothing throws.

## Requirements
- Local players with an override and bots get exactly the same action.
- Turn limit 0 aims straight; a limit is never exceeded in one tick.
- Pure: no process, no wiring, no clock (checked by `Tools/Tests/test_components.py`).

## Tests
`ActionTests` and `LockstepTests` in `Artemis.Tests`; in game, `Agent/scripts/bridge_check.py run` and `turn`.
