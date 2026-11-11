# Artemis.Engine.Control

Group `5_Engine` - static library inside the game process - applies the actions of the players and runs the bots.

## Responsibility
- `PlayerActionDetour`: once per tick on the game thread, writes the overrides into the action of any player (local or bot) and
  publishes the actions. It asks the `ActionSource` port for the overrides of the client's players.
- Virtual input: `InputUpdateDetour` gives MCC the state of a virtual gamepad for local players switched to Artemis.
- Bots: players on machines only Artemis knows, added like players that join in progress (`BotService`, `SetMachinesDetour`) and
  removed with the "left" and "remove" player updates of the engine (LS.1, D284); it also implements `Control`'s `UnitRules` with the
  engine functions.

## Does not
- Decide or build the action: that is `Control`.
- Know the bridge: it sees it only through `ActionSource`, which `Mod` implements.

## Input and output
- Input: overrides set by `ActionSource::Prepare`, bot, team and remove requests, virtual input states.
- Output: writes into the engine's player actions and input; the published actions and session snapshot.

## Public interface
Other projects may import only these modules; everything else in the project is internal.

| Module | What it gives |
|---|---|
| `Engine.Control.Layer` | The facade: input, action and bot stores, services and detours. Owner only. |
| `Engine.Control.Input.Type`, `Engine.Control.Input.State` | Input sources and states, `VirtualInputStore`, `PlayerActionStore`. |
| `Engine.Control.Input.System` | The `ActionSource` port and the input services. |
| `Engine.Control.Simulation.Type`, `Engine.Control.Simulation.State` | Machines and players of the simulation, the pure `Roster` and `PlanRoster` (bots to move, remove and add to reach a count per team), `FindFreeBotNumber` (lowest free bot number), `MakePlayerUpdate` (buffer of the player update), `BotStore` (bot, team and remove requests, session snapshot). |

## Depends on
`Common`, `Services`, `Engine.Base`, `Control`, `Blam.Layout`.

## State between ticks
The overrides and action history of every controlled player, the virtual input of each local player, the bots and their machines.

## Threads
Game thread for the hooks and the engine functions. `ActionSource::Prepare` may block there in Lockstep: at most
`k_LockstepTimeout` per wait, no limit during a pause of a live client, at once when the client leaves. The stores are thread-safe
for the UI and the bench.

## Errors
An engine function with a changed signature logs an ERROR, never silently. Without the player update function (0x418ff8) bots are
still added but never removed. A bot that cannot be removed (the engine refuses a step, the slot stays taken) logs the step and stays
in the `BotStore`; a retry resumes where it stopped. Only the bots Artemis added are removed, never a local or real player. `PlayerActionDetour` without an action source does not
install. A lost client gives its players back to the engine.

## Requirements
Local players with an override and bots receive the same action.

## Tests
`EngineLayerTests` (builds without the game; refuses to install the action hook without a source) and `BotRosterTests` (roster plan,
bot numbers, update buffer, remove requests) in `Artemis.Tests`. In game:
`Agent/scripts/bridge_check.py` (`run`, `turn`) and the bots stress scenario of the bench.
