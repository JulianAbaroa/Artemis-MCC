# Artemis.Engine.Control

Group `5_Engine` - static library inside the game process - applies the actions of the players and runs the bots.

## Responsibility
- `PlayerActionDetour`: once per tick on the game thread, writes the overrides into the action of any player (local or bot) and
  publishes the actions. It asks the `ActionSource` port for the overrides of the client's players.
- Virtual input: `InputUpdateDetour` gives MCC the state of a virtual gamepad for local players switched to Artemis.
- Bots: players on machines only Artemis knows, added like players that join in progress (`BotService`, `SetMachinesDetour`); it
  also implements `Control`'s `UnitRules` with the engine functions.

## Does not
- Decide or build the action: that is `Control`.
- Know the bridge: it sees it only through `ActionSource`, which `Mod` implements.

## Input and output
- Input: overrides set by `ActionSource::Prepare`, bot and team requests, virtual input states.
- Output: writes into the engine's player actions and input; the published actions and session snapshot.

## Public interface
Other projects may import only these modules; everything else in the project is internal.

| Module | What it gives |
|---|---|
| `Engine.Control.Layer` | The facade: input, action and bot stores, services and detours. Owner only. |
| `Engine.Control.Input.Type`, `Engine.Control.Input.State` | Input sources and states, `VirtualInputStore`, `PlayerActionStore`. |
| `Engine.Control.Input.System` | The `ActionSource` port and the input services. |
| `Engine.Control.Simulation.Type`, `Engine.Control.Simulation.State` | Machines and players of the simulation, `BotStore` (bot and team requests, session snapshot). |

## Depends on
`Common`, `Services`, `Engine.Base`, `Control`, `Blam.Layout`.

## State between ticks
The overrides and action history of every controlled player, the virtual input of each local player, the bots and their machines.

## Threads
Game thread for the hooks and the engine functions. `ActionSource::Prepare` may block there in Lockstep: at most
`k_LockstepTimeout` per wait, no limit during a pause of a live client, at once when the client leaves. The stores are thread-safe
for the UI and the bench.

## Errors
An engine function with a changed signature logs an ERROR, never silently. `PlayerActionDetour` without an action source does not
install. A lost client gives its players back to the engine.

## Requirements
Local players with an override and bots receive the same action.

## Tests
`EngineLayerTests` (builds without the game; refuses to install the action hook without a source) in `Artemis.Tests`. In game:
`Agent/scripts/bridge_check.py` (`run`, `turn`) and the bots stress scenario of the bench.
