# Artemis.Control

Group `3_Domain` - pure static library - the logic of the player action and of the bridge's lockstep, without touching the game.

## Responsibility
- Action: from the decision of an agent to the fields Artemis writes into the 0xA8-byte action of a tick (buttons, hold lanes with
  the engine's threshold, limited turn of the aim, weapon/grenade/zoom requests of a client).
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
| `Control.Action.System` | `BuildAction`, `GetHoldThresholdTicks` and the `UnitRules` port. |
| `Control.Lockstep.Type` | Limits and defaults of the lockstep (decision period N, latency d, timeouts) and `BridgeControlState`. |
| `Control.Lockstep.System` | `DecideControl`, `ApplyConnection`, `BuildOverride`, `GetDueDecision`, `RecordWait`, `ResumeIfAnswered`, `GetApplyTiming`, `GetReleasedOverride`, `ReadClock`, `IsDecisionTick`. |

## Depends on
`Common`, `Blam.Layout`, `Contract`.

## State between ticks
None of its own: the caller keeps the `PlayerActionHistory` of each player and the bridge control state, and passes them in.

## Threads
Called on the game thread (the PlayerAction hook builds the action) and on the bridge thread (control). Pure functions: they never
block or allocate on the hot path.

## Errors
Out-of-range requests are clamped (N, d) or sanitized (`BuildOverride`); nothing throws.

## Requirements
- Local players with an override and bots get exactly the same action.
- Turn limit 0 aims straight; a limit is never exceeded in one tick.
- Pure: no process, no wiring, no clock (checked by `Tools/Tests/test_components.py`).

## Tests
`ActionTests` and `LockstepTests` in `Artemis.Tests`; in game, `Agent/scripts/bridge_check.py run` and `turn`.
