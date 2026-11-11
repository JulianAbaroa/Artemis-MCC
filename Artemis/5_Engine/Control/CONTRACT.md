# Artemis.Engine.Control

Group `5_Engine` - static library inside the game process - applies the actions of the players and runs the bots.

## Responsibility
- `PlayerActionDetour`: once per tick on the game thread, writes the overrides into the action of any player (local or bot) and
  publishes the actions. It asks the `ActionSource` port for the overrides of the client's players.
- Virtual input: `InputUpdateDetour` gives MCC the state of a virtual gamepad for local players switched to Artemis.
- Bots: players on machines only Artemis knows, added like players that join in progress (`BotService`, `SetMachinesDetour`) and
  removed with the "left" and "remove" player updates of the engine (LS.1, D284); it also implements `Control`'s `UnitRules` with the
  engine functions.
- Roster of the bots (LS.1, D284): the roster is the number of bots per team plus the ones that go to the team of the local player 0 (the local players are not part of it). `BotService::Update` applies it on the game thread: at the first tick of the match (a request made before it, in the lobby, is the roster of that load, otherwise the active one is applied) and, for a request made in the match, once at the first tick whose round state (`ReadRoundState`) is ended or between rounds, never in the middle of a round. A roster that does not fit (more than `k_MaxBots` or than the free slots of the player table) is refused whole with a WARNING. A request becomes the active roster only once it was applied (at load or between rounds), a refused one leaves the active roster as it was. `Rejected` can also mean partly applied: when a move, a remove or an add fails there is no rollback and no retry, the operations done stay and the reason says "k of n operations done" (the ones skipped after a failed add count as not done). Each request has a sequence number and `BotStore::FinishRoster` takes the result and the new active roster in one step, so the result of an older request never replaces the one of a newer request. The active roster and a pending request survive `Cleanup` (the next match applies them at its first tick) but the result goes back to neutral (Pending if a request waits), and the bots do not survive. Every removal goes through `BotService::RemoveBot`, which also clears the action override of that player index in `PlayerActionStore`. The owners of per-player state (the Workbench action mode, the bench modes) key it by the handle of the player, whose salt changes when a new bot takes the index, so they need no removal event. The result (`RosterResult`: pending, at load, applied between rounds R, rejected with the reason) is in `BotStore`. If `PlayerActionDetour` does not run during the break between rounds the request waits for a tick where the state is read again: the log line "Roster applied between rounds R" says when it happened.

## Does not
- Decide or build the action: that is `Control`.
- Know the bridge: it sees it only through `ActionSource`, which `Mod` implements.

## Input and output
- Input: overrides set by `ActionSource::Prepare`, bot, team and remove requests, roster requests, virtual input states.
- Output: writes into the engine's player actions and input; the published actions and session snapshot.

## Public interface
Other projects may import only these modules; everything else in the project is internal.

| Module | What it gives |
|---|---|
| `Engine.Control.Layer` | The facade: input, action and bot stores, services and detours. Owner only. |
| `Engine.Control.Input.Type`, `Engine.Control.Input.State` | Input sources and states, `VirtualInputStore`, `PlayerActionStore`. |
| `Engine.Control.Input.System` | The `ActionSource` port and the input services. |
| `Engine.Control.Simulation.Type`, `Engine.Control.Simulation.State` | Machines and players of the simulation, the pure `Roster` and `PlanRoster` (bots to move, remove and add to reach a count per team), `RosterResult`, `GetRosterStep` (what the roster service does on a tick), `ToText`, `FindFreeBotNumber` (lowest free bot number), `MakePlayerUpdate` (buffer of the player update), `BotStore` (bot, team, remove and roster requests, the active roster and its result, session snapshot). |

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
