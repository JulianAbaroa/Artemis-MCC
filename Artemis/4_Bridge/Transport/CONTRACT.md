# Artemis.Bridge.Transport

Group `4_Bridge` - static library with side effects (shared memory, events, discovery file) - the bridge with the Python client.

## Responsibility
The transport to a Python client on the same PC: the region `Local\Artemis.Bridge.<pid>` with its two events and the discovery
file in `%LOCALAPPDATA%`; the control block (phase, owner, mode, N, d, heartbeats, counters); two `BridgeTick` buffers with a
seqlock; one action slot per player; and the lockstep of the actions on the game thread. It declares the `BridgeEngine` port for
what it needs from the engine.

## Does not
- Import any engine project: it reaches the engine only through `BridgeEngine`, implemented by `Mod`.
- Flatten ticks (`Bridge.Codec`) or decide the lockstep policy (`Control.Lockstep.System`).
- Apply actions: `Engine.Control` asks its `ActionSource` port, which `Mod` implements over `BridgeService`.

## Input and output
- Input: flattened decision ticks, the client's control requests and actions.
- Output: published ticks, per-player overrides and client requests through `BridgeEngine`, the control in effect.

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
- AI thread: captures the clock while the game waits for the read of a tick.
- Game thread: applies the actions in the PlayerAction hook and, in Lockstep, waits for them: at most `k_LockstepTimeout` per
  decision; two timeouts in a row suspend Lockstep until the client answers; no limit while paused and the client is alive; the wait
  ends at once if the client leaves or the bridge stops.

## Errors
A region that cannot be created leaves the bridge off; the game is not affected. A client that leaves or crashes: its players go
back to the engine, bots stand still, the UI owns the instance again.

## Requirements
Same measured behavior as before: 0 lost events and 0 timeouts in the stage-0 run, pauses of 120 s, `bridge_check` PASS.

## Tests
`TransportTests` in `Artemis.Tests` (a client in the test process: real region and events). In game: `Agent/scripts/bridge_check.py`
(`run`, `turn`, pauses, `free`, client crash) and the stage-0 smoke test.
