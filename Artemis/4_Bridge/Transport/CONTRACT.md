# Artemis.Bridge.Transport

Group `4_Bridge` - static library with side effects (shared memory, events, discovery file) - the bridge with the Python client.

## Responsibility
The transport to a Python client on the same PC: the region `Local\Artemis.Bridge.<pid>` with its two events and the discovery
file in `%LOCALAPPDATA%`; the control block (phase, owner, mode, N, d, heartbeats, counters); two `BridgeTick` buffers with a
seqlock; one action slot per player; and the lockstep of the actions on the game thread. It declares the `BridgeEngine` port for
what it needs from the engine. The match setup request (LS.2, D286; before it the preset, Q97, and the map variant, Q98, had a request each) follows the end of round one: the `RequestSetup` counter with the fields it names in `RequestSetupFields` (preset, map, roster), taken only while Python owns the instance (the fields are written before the counter is raised and read after it is loaded with acquire) and echoed in `SetupSequence`, which is written last with a release store, after `SetupResult`, `SetupRound`, `SetupReason` and the active setup (`ActivePreset`, `ActiveMap`, `ActiveRoster`). A counter raised without ownership is echoed and dropped, not queued. A request that still waits for a round break is dropped, once, when Python loses the instance (`CancelSetupRequest`), as the end of round one is, and the engine drops only a request of the bridge. `BridgeEngine` has `RequestSetup(BridgeSetupRequest)`, `CancelSetupRequest`, `GetSetupResult` and `GetActiveSetup`. The names in a request are resolved on the bridge thread (the files and the block of the named configuration are read there), never on the game thread; a field the mask does not name is absent in the request and means no change, and an empty name with its bit set asks for the base values (preset) or the map the match loaded with (map).

The timeline of a decision (P.3 C, D373): `BridgeService::ReadQpc` is the one QueryPerformanceCounter read, the clock of `PublishQpc` and of every mark. `CaptureClock` sets `CaptureQpc` and copies the Need block of the hook state; the AI thread sets `AiStartQpc` and `AiEndQpc` and the bridge thread `FlattenStartQpc` on the `BridgeClock` that travels with the tick; `EndTick` copies them to the `BridgeTickHeader`. `Apply` measures, at the tick t + d of a decision in Lockstep (the rule is `Control.Lockstep.System::GetNeedDecision`), `NeedQpc` on arrival and `NeedEndQpc` when the answer was available (the same instant without a wait, the end of the wait otherwise, also a cut one) with `NeedFlags` (waited, timed out, pause seen, cut). `Apply` of t + d runs before the capture of the tick t + d, so that tick already carries its Need. All 0 in a `.tdump`.

The actions (VW.9c.3 D, D365): `ApplyDecision` turns the `BridgeAgentInput` of each answered slot into an `AgentInput` with `Control.Lockstep.System::ToAgentInput` (the one place that makes it sane, a changed value counts in `RejectedActions`) and gives it to the engine with the handle of the slot (another handle drops the action and counts in `HandleMismatches`), the window d (`Latency`) and the turn limit of the control block. The decision t is applied at t + d (Lockstep or Free, D304) and the client of Artemis plays it during the window. A player the client lets go of is `ClearAgentInput`ed when it is local, and a bot gets an `AgentInput` that asks for nothing (window 1), so it stands still with the facing it had. `Apply` also asks the engine to close the window of the possessed player where `IsImitationWindowEnd` says (d ticks counted from the decision tick), and `CaptureClock` reads the possessed player and the last closed window together (`GetImitation`, game thread, so they are of the same moment) into the `BridgeClock`, which `EndTick` writes into the header (`PossessedPlayer`, `PossessedInput`). The window that `Apply(t)` closes covers the action ticks (t - d, t]; with N greater than d the last window of a period is shorter when d does not divide N, and in sparse mode Python sees only that last one (the pairing with the action window of an agent, [t + d, t + 2d), is a decision for F5 B6). Python receives it and does not use it yet. The last setter wins in the `PlayerActionStore`: an override of the bench or the Viewer on a Python player takes its `AgentInput` away until the next decision (at most N ticks), then Python plays again.

## Does not
- Import any engine project: it reaches the engine only through `BridgeEngine`, implemented by `Mod`.
- Flatten ticks (`Bridge.Codec`) or decide the lockstep policy (`Control.Lockstep.System`).
- Apply actions: `Engine.Control` asks its `ActionSource` port, which `Mod` implements over `BridgeService`.

## Input and output
- Input: flattened decision ticks, the client's control requests and actions.
- Output: published ticks, an `AgentInput` per agent player (`BridgeEngine::SetAgentInput`, `ClearAgentInput`) and the closing of the imitation window (`CloseImitationWindow`) through `BridgeEngine`, the control in effect.

## Public interface
Other projects may import only these modules; everything else in the project is internal.

| Module | What it gives |
|---|---|
| `Bridge.Transport.Layer` | The facade: `BridgeStore` and `BridgeService`. Owner only: no callbacks, installs nothing. |
| `Bridge.Transport.System` | `BridgeService` and the `BridgeEngine` port. |
| `Bridge.Transport.Type` | Timeouts (client, lockstep, wait slice, control interval) and the slot and link types. |

## Depends on
`Common`, `Services`, `Contract`, `Control`, `Blam.Layout`, `Bridge.Codec` (and, through it, the pure projects it references).

## State between ticks
The session with the client (heartbeats, owner, counters, suspension after timeouts) and the clock of the action ticks.

## Threads
- Bridge thread: opens and closes the region, runs `Update`, publishes ticks.
- AI thread: captures the clock while the game waits for the read of a tick, with whether the tick is a decision and whether the mode is sparse (`Control.Lockstep.System::IsSparse` over the control and the every-tick demand of the settings, Q101). The bridge thread publishes the effective value as `IsSparse`.
- Game thread: applies the actions in the PlayerAction hook and, in Lockstep, waits for them: at most `k_LockstepTimeout` per
  decision; two timeouts in a row suspend Lockstep until the client answers; no limit while paused and the client is alive; the wait
  ends at once if the client leaves or the bridge stops. It waits only for decisions up to the last one marked with `MarkPushed`, which means
  queued for the AI thread, not delivered to the client (RV.1): a decision the producer lost is never marked, and if the AI thread hangs with a
  Lockstep client connected the game waits `k_LockstepTimeout` per decision until two timeouts suspend it.
- The wait between ticks (TG.1, D380): before each tick of `SimulationTicks` the tick gate calls `BridgeService::HoldTick` on the game thread. A pause
  requested ends the frame at once (true), with or without a decision pending. Otherwise, if the next tick (`ActionTick + 1`) has to wait for a decision
  nobody answered, it opens the hold (`BridgeHookState::Hold`) and waits at most `k_LockstepHoldBudget` (4 ms) by the rule `DecideHold`: true ends the
  frame so the engine draws and the next frame asks again, false lets the tick run (answered, cap `k_LockstepTimeout` passed, client or bridge gone). The
  tick gate predicts the action tick, and if the engine repeats it the wait happens in the tick as before, so the wait in `Apply` stays as the fallback.
  `BridgeHold::ActionTick` is the tick the gate predicted: `Apply` continues the hold only when its action tick is that one, and clears it only from that tick on. `HoldTick` counts nothing: `Apply` continues the hold of its decision (the wait starts at `Hold.Start` with `Hold.Deadline` and `Hold.IsPauseSeen`, `NeedQpc` is
  `Hold.StartQpc`) and counts one wait, or one timeout, per decision exactly as without the gate (a wait that saw a pause is not a cost), then clears the hold.
  `BeginMatch` clears it with the rest of the hook state.

## Errors
A region that cannot be created leaves the bridge off; the game is not affected. A client that leaves or crashes: its players go
back to the engine, bots stand still, the UI owns the instance again.

## Requirements
Same measured behavior as before: 0 lost events and 0 timeouts in the stage-0 run, pauses of 120 s, `bridge_check` PASS.

## Tests
`TransportTests` in `Artemis.Tests` (a client in the test process: real region and events). In game: `Agent/scripts/bridge_check.py`
(`run`, `turn`, pauses, `free`, client crash) and the stage-0 smoke test.
