# Artemis.Engine.Control

Group `5_Engine` - static library inside the game process - applies the actions of the players and runs the bots.

## Responsibility
- `PlayerActionDetour`: once per tick on the game thread, writes the action of any player (local or bot: an override of the bench or the Viewer, or what the client of Artemis makes of an AgentInput) and
  publishes the actions. It asks the `ActionSource` port (the bridge, which sets the `AgentInput` of Python's players in the `PlayerActionStore` through `BridgeEngine`) before it reads the store. After the engine applied the actions it
  also calls the engine function `CopyActionToView` (0x62fcc) on the game thread for every view that shows a non local player, outside the
  theater and only if that player has an action in the tick (after the overrides), as the engine does in the theater: the camera, zoom and
  weapon of a view that watches a bot (Watch POV) follow it tick by tick (VW.9a.y, D349). The decision is the pure `FindViewActionPlayer`. A
  view of a local player is never touched (the engine moves it every frame) and without POV no view shows a non local player, so it is
  never called.
- `ClientOracleDetour` (VW.9c.1, D356): development oracle, off unless the shadow check switch of `PlayerActionStore` is on. It hooks the player
  control of the engine (0x5df80, found by its rva with the control tag anchor and the unwind table, no prologue bytes) and, for a local human
  player without an override, copies the event table (0x700 bytes), the client block and the output at the entry, calls the original, runs
  the pure `StepClient` on the copy and compares output and state bit by bit (`CompareClient`). Counters are the `Client ...` fields of the
  shadow check (calls compared, calls that differ, by output field and by context: on foot, seated, zoomed, no unit; calls not compared).
  A call that read a unit fact the context does not know (`UnitContext::Known`, `ClientOutput::UnknownRead`) is NOT counted in those fields:
  it only goes to `Client partial`, so a gap in the context is never counted as a bug of the port (VW.9c.2: the bot service resolves every
  `UnitFact`, see `UnitContext` below, so a call is partial only when an engine function was not found or a read failed). The first differences go to the log. Every difference is a line of `%LOCALAPPDATA%\Artemis\client_oracle.txt`
  (`diff user= dt= unknown= move= look= trig= tail= buttons= events= lanes= state= table= block=`, the table and the client block as hex) to turn
  into an offline test. The game thread never does file I/O: the hook only formats the line into a bounded buffer (`k_ClientBufferLines`, a
  full buffer drops the line and counts it, `k_ClientMaxFileLines` per session) and the Main thread writes it (`Flush`, every second from
  `MainThread`, and on `Uninstall`). It writes nothing of the game. With the switch off the hook costs one atomic load. The control tag
  locator, the input user to player search and the thread context read are shared with `PlayerActionDetour` (`Engine.Control.Input.System`).
- Possession of a bot (VW.9c.2 B, D350-D354, D358-D361): the client of the user runs per FRAME and the ticks copy its result. The hook of the player
  control (0x5df80, `ClientOracleDetour`) is a PRODUCTION hook now: it is installed always (the oracle part stays off behind the shadow check
  switch), and if the hook or its anchors are not found it logs one ERROR and no bot can be possessed. For input user 0, while a bot is possessed, it
  calls `PossessionService::StepFrame` (`Engine.Control.Input.System`, the hook only detours) with the event table of that frame, the dt of the frame
  and the settings of the user (`ReadClientSettings`, D359). `StepFrame` runs the pure `StepClient` against the client state of the BOT (its unit and
  zoom, `GetUnitClientState` and `GetClientContext`, user level facts from the player of input user 0, no turn limit, D358) and the pure
  `AccumulateFrame` adds it to the `ClientDriven` of the `PossessionStore`: the look goes into the absolute aim (`IntegrateAim`), the output becomes the held
  state and what lasts one frame (buttons, lanes, pulse events `k_EventPulses`, triggers) waits in `Pending`. Every frame counts once whatever the
  ticks do, so 0, 1 or several ticks per frame lose and repeat nothing. `PlayerActionDetour::ApplyPossession` runs once per tick, after the overrides
  are copied: it resolves the request (`PossessRequestStore`, one atomic word of this project because `Engine.Control` does not import Pov, written by the
  Viewer render thread, read with `k_PossessRequestMaxAge`, 100 ms like Watch POV, signed age) against `BotService::GetBotPlayerHandles` (only an Artemis
  bot, never a human, the salt must match: pure `FindPossessedBot`) and the pure `TakeTickAction` copies the accumulated aim, the held state and the
  pending pulses to the action, clearing them (delivered once), as 0x628b0 copies the block. The result goes through the SAME path as an agent
  (`MakeClientAction`: `ActionOverride` plus `ClientRequests`; `BuildAction`, `UnitRules`, hold lanes D121, interaction D118 and the unit slot are the
  existing ones). The `AgentInput` of Python for the possessed bot is not played while it is possessed (D351, D367). While a bot is possessed the human local player of input user 0 gets a
  neutral override (`MakeNeutralOverride`, no write to its control block, D361) whether it has a unit or not, only when it has an engine action and no
  override of an agent (a local agent is never touched, D350): pure `ShouldNeutralizeLocal`. No request, a human or a gone bot releases the possession the
  same tick. A tick whose unit is missing, or that no frame has served yet, gives the bot a neutral action and counts; one WARNING per possession for a
  missing unit. `ReadClientSettings`, `ReadDualWieldRestrictions` and `ReadInputTable` (`Input.System:ClientSource`) are what the client of the engine reads
  about a user: the oracle and the possession both use them. `UnitContext` never passes an input user outside 0..3 to 0x61830. A request is dropped when
  the Viewer stops publishing for more than 100 ms (stalled Presents), which releases the bot. The bot is shared by the frame thread and the tick thread without a lock, so `StepFrame` checks that it runs on the thread that began the possession (`PossessionStore::IsTickThread`): on another thread it logs one ERROR and `Disable`s the possession. A bot with no new frame for more than `k_PossessMaxStaleTicks` ticks gets a neutral action and forgets its held state (`TickFrameAge`). `SetPossession` of both detours connects the stores
  (before Install: without it nobody is possessed). The tick marks the possessed player: see `Bridge.Codec` (and in the bridge since VW.9c.3 D). Imitation (D364): `PossessionService::StepFrame` also adds each frame it ran to the `AgentInputReducer` of the `PossessionStore`, and `PossessionService::CloseWindow` (game thread, called by the bridge through its engine port where a window of d ticks ends) turns what was added into the last closed window (`CloseAgentInput`), which `PossessionStore::GetWindow` gives back: the level and the edge of each control (a frame with down count 1 is an edge), the move axes of the last frame and the look the client was asked in the window, before zoom and damp. The reducer and the window start empty with each possession and are forgotten when it ends.
- Agents through the client of Artemis (VW.9c.3 C and D, D362, D363, D367; the bridge sets them through `SetAgentInput` since D2, v2): `PlayerActionStore` holds per player an `AgentDelivery` (the player handle,
  the `AgentInput`, the window length d in ticks, the turn limit per tick and a sequence number that rises with every `SetAgentInput(index, handle, ...)`), under the same lock as the overrides. It is EXCLUSIVE
  with an `ActionOverride`: setting one removes the other (the last wins), releasing an override (`SetOverride` with nothing) leaves the AgentInput, `ClearAgentInput`
  stops it (with a handle, only the AgentInput of that handle), `BotService::RemoveBot` and `Cleanup` drop it; a turn limit that is not finite or is negative is kept as 0 (no limit). The hook copies both
  in one lock (`CopyActions`), so in one tick a player has one or the other. Every tick `PlayerActionDetour::ApplyAgents` (after the overrides are copied and the possession resolved,
  before `ApplyPossession`) only wires: it reads the settings once and calls `AgentClientService::PlayTick` for each player, which decides with the pure `PlanAgent`
  and counts every tick. A player whose AgentInput is of another handle than the one now in its index (or whose slot is empty) is Stale: its client ends and the
  delivery is dropped (`ClearAgentInput` with that handle, so a new delivery of the new player stays). A possessed player is Suspend (possession wins, D351: no step,
  and the AgentInput waits in the store). Any other player with an AgentInput is Run, one step of `AgentClientService`: `AdvanceAgentWindow` (a sequence number not seen before is
  tick 0 of a window, any other tick is one more), `GetAgentTickInput` (past the window with no new delivery the same held controls and axes repeat with Edge 0 and no look:
  what is held and the axes go on, a tap is over and nothing turns), `MakeAgentFrame` (synthetic frame), `StepClient` against the unit of the player with
  `ReadAgentClientSettings` (the settings of Artemis, D359), `AccumulateFrame` and `TakeTickAction` in the same tick (one frame per tick), so the result is an
  override plus client requests like the possession's and follows `BuildAction` and `WriteInteraction` (D118-D121). The dt is the seconds of one tick of the engine clock
  (`ClientSettings::TickSeconds`, not real time, D105). A player without a unit gets the neutral action and forgets its held state, a new unit (respawn) starts the aim
  from the unit and presses the held controls again, and the window and the last delivery seen survive a missing unit. It writes no control block of any local player
  (the override of an agent has no local index). The state of each agent is in `AgentClientStore` (game thread only, begun when the player gets an AgentInput and
  ended when it loses it, its handle changes or the match ends, and suspended when it is possessed), its counters (ticks, ticks with a step that ran, ticks with Unsupported, ticks with UnknownRead) are atomics that the Viewer
  shows under the possession line, and one WARNING per agent is logged the first time a step is Unsupported or reads an unknown unit fact.
  The settings of an agent (`ReadAgentClientSettings`, one function): the game facts (hold threshold, zoom cancel, jump timer, ticks per second, tick seconds) are read
  from the control tag and the clock like for a user, and every preference is fixed and never copied from a user: aim scales 1, crouch is a hold, the jump is not a
  flag, melee does not hold use, game system, session and input ready, not a menu. The `ClientContextSource` of an agent: thread context of the tick, no input user
  (`k_NoInputUser`, `k_NoLocalIndex`: `BuildUnitContext` then says the controls are free, `ControlLocked` known and false, and never asks the engine), `PlayerHandle`
  of the player of the agent (look rates, look offset, alternate control and boost profile are the player's own), `ClientZoomLevel` of the state, dual wield
  restrictions 0, boost values the `Value` of events 0x0A and 0x26 of the synthetic frame.
  VW.9c.3 D, D367 (Q112): (1) the end of a possession goes on in REPETITION: `AgentClientStore::Suspend` keeps only the last sequence seen with its window over, so the first
  tick after the release has Edge 0 and no look and the held controls and axes as they were, until the next delivery; the old window is never played again (it would repeat a
  jump, a grenade or a melee). `End` (no delivery, a new handle, the end of the match) forgets the sequence, and a delivery that came during the possession is a new window. The
  synthesizer (held controls) DOES survive the suspension (`AgentClient::ResumedUnit`, user 2026-11-23, M1 of the review): a Level held before the possession is still held
  when it ends, with its down count going on, so it is not pressed again (no jump, grenade or melee). If the unit changed during the possession it is a new life and
  `PrepareAgentFrame` forgets the controls as for a respawn. (2) The window advances on EVERY tick the agent plays,
  with or without settings or unit. A step that does not run (`ClientStatus` not `Ran`) does not spend the synthesizer: `UndoAgentFrame` gives back what the controls counted
  on that tick (down counts, held ticks, down mask) and keeps the start mask of the window, because the window itself goes on counting, so a tap whose tick did not run is lost
  and is not moved to the next one. The same for a re-press (Level and Edge of a control already held, released at tick 0 and pressed at tick 1): when tick 0 does not run
  nobody saw the release, the down count goes on and tick 1 is a hold, not a press. (3) `AgentClientStore::Cleanup` runs where `PlayerActionStore::Cleanup` runs (end of match, any thread): the counters and the active mask
  go at once, the state of the clients is dropped by the game thread at its next call (an atomic flag, no lock), which zeroes the counters and the mask again. (4) An agent is per HANDLE: `AgentDelivery::Handle` and
  `ClientDriven::Handle` (filled for agents too) are the handle with the salt, and a delivery for another handle in the index is dropped. The bench keeps its applied inputs by
  handle and clears by handle. (5) The client requests the store holds for a player with an AgentInput (the bench `request` line, `RequestClient`) ADD UP (`operator|` of
  `ClientRequests`) to what its client asks the same tick, they are not lost. (6) `AgentClientService` owns Begin / Suspend / End and all the counting (`PlayTick`, `SuspendAll`: a tick the hook does not run actions suspends every agent, it never ends them, so the old window is not played again);
  `ClientAction` / `MakeClientAction` are the one name for what the client gives a player, a possessed bot or an agent.
- Unit facts of the client (VW.9c.2, D356): `BotService::GetClientContext(state, source, settings, dt)` resolves every `UnitFact` of the unit it is
  given (the local's in the oracle, the bot's in the possession) as far as the memory and the functions answer: what it cannot resolve is out of
  `Known` and the step says so. The pure `BuildUnitContext` (`Engine.Control.Simulation.System:UnitContext`)
  decides each fact over the `UnitFactPort` (object, player and tag memory reads and the engine calls), so it is tested with bytes. A read or a
  call that cannot be done leaves the fact out of `UnitContext::Known`, never a guessed value. `BotService` implements the port (`EngineUnitFacts`):
  memory by handle (`GetObjectData` rules: the low 16 bits index the object, the thread context is the one of the game thread), tag memory
  through the two tag globals decoded from the first bytes of the tag flag test (`DecodeTagGlobals`, `k_TagFlagTest`: the tag data of a tag
  is `segment[index >> 28] + index * 4`), and ten engine functions found by signature in `ResolveFunctions` (zoom magnification 0x4856b8, move
  penalty 0x4bc51c, crouch gate 0x4ac810, forced crouch 0x4abee8, crouch toggle blocked 0x48be48, next weapon zoom level 0x4bc624, frame of
  reference 0x5dddc, controls locked 0x61830, look offset 0xda824, alternate control 0x715ac). All of them only read (the look offset and
  frame functions write their own outputs into buffers of the port). A function not found is one ERROR and only its facts stay unknown.
  The user-level facts (controls lock, look offset, alternate control, look rates, dual wield restrictions) are asked with the ids of the
  `ClientContextSource` the caller gives: the oracle fills it from its call (`GetPlayerHandle`, the zoom level of the client block, the
  restrictions byte read by rva by `ReadDualWieldFlags`; without the module base it is absent and DualWield stays unknown), plus the
  `ClientSettings` and the dt of the call. A unit in a seat reads the trigger curve from the parent tag (seat flag bit 2) and the zoom item from
  the parent weapon (seat flag bit 3), like 0005.c. A unit that is not a biped has no jump assist object. Game thread only.
  The boost (VW.9c.2 C): `EngineUnitFacts::EvaluateTriggerCurve` runs the tag function at +0x498 of the tag with the two engine functions 0x174ed8
  (evaluate) and 0x175524 (scale) of `UnitFunctions`, only for a raw value that is not zero, once per call of a unit with the trigger curve bit, on the
  game thread. The raw value comes from the `ClientContextSource` (events 0x0A and 0x26 of the table of the call, both read by the oracle and the
  possession). `IsBoostAltEvent` (0x70ed0) is read from memory: the game engine globals +0x1754 + player index * 300 equals 1, with the player of the
  source (without one the default event). The functions are not ported: without them the value stays outside `Known`.
- Virtual input: `InputUpdateDetour` gives MCC the state of a virtual gamepad for local players switched to Artemis.
- Bots: players on machines only Artemis knows, added like players that join in progress (`BotService`, `SetMachinesDetour`) and
  removed with the "left" and "remove" player updates of the engine (LS.1, D284); it also implements `Control`'s `UnitRules` with the
  engine functions.
- Roster of the bots (LS.1, D284; the request moved to the match setup in LS.2, D286): the roster is the number of bots per team plus the ones that go to the team of the local player 0 (the local players are not part of it). It is one field of `MatchSetup` (`Engine.Base.MatchSetup`, whose `Roster` type is the one used here) and `BotService` no longer keeps requests or results: it implements `RosterPort` (`CheckRoster` and `ApplyRoster`) and `MatchSetupService` decides when. `PlayerActionDetour` calls `UpdateLoad` on the game thread before `BotService::Update`, at the first tick of the match (the roster of the load, or the active one). The rest is `UpdateBreak`, which `SimulationTicksDetour` (Engine.Capture) calls before the ticks of every frame: a request made in the match is served once at the first frame whose round state (`ReadRoundState`) is ended or between rounds, never in the middle of a round, and a request that waited at the first tick is served in the first frame after it (AtLoad). An immediate request (roster only, from the Workbench "Apply now", U.1, D289) is served by `UpdateBreak` in the first frame after the first tick, in the middle of a round if need be, with the same `CheckRoster` and `ApplyRoster`. The roster is the last thing of a request, after its preset and map variant, and `Mod` connects `BotService` as the port (`SetRosterPort`). The engine functions of the roster run from `SimulationTicksDetour` in the break: no document says they need `PlayerActionDetour`, but it is not verified in game. A roster that does not fit (more than `k_MaxBots` or than the free slots of the player table) is refused whole with a WARNING. A roster becomes the active one only once it was applied (at load or between rounds), a refused one leaves the active roster as it was. `Rejected` can also mean partly applied: when a move, a remove or an add fails there is no rollback and no retry, the operations done stay and the reason says "k of n operations done" (the ones skipped after a failed add count as not done). The result and the active roster live in `MatchSetupStore` (one step, with the sequence number of the request, so the result of an older request never replaces the one of a newer request); they survive `Cleanup` of the match (the next match applies them at its first tick), the result going back to neutral, and the bots do not survive (`BotStore::Cleanup`). Every removal goes through `BotService::RemoveBot`, which also clears the action override of that player index in `PlayerActionStore`. The owners of per-player state (the Workbench action mode, the bench modes) key it by the handle of the player, whose salt changes when a new bot takes the index, so they need no removal event. The result (`MatchSetupResult`: pending, at load, applied between rounds R, applied now round R for an immediate request, rejected with the reason) is in `MatchSetupStore`. If `PlayerActionDetour` does not run during the break between rounds the request waits for a tick where the state is read again: the log line "Setup N applied between rounds R" (or "applied now, round R" for an immediate request) says when it happened.

`PlayerActionStore` also implements the `AppliedActionsPort` of `Engine.Base` (D371): `CopyApplied` copies the `Sequence`, the mask and the 16 raw actions of the latest published tick (the ones `PlayerActionDetour` published right before the engine applied them, agent overrides included) into an `AppliedActions` for the capture of `Engine.Capture`, under the store mutex, false before the first tick. It only copies: the layout is the capture's.

## Does not
- Decide or build the action: that is `Control`.
- Know the bridge: it sees it only through `ActionSource`, which `Mod` implements.

## Input and output
- Input: the `AgentInput` or override set in the `PlayerActionStore` (by `ActionSource::Prepare` for Python's players, by the bench and the Viewer), bot, team and remove requests, roster requests, virtual input states.
- Output: writes into the engine's player actions and input; the published actions and session snapshot.

## Public interface
Other projects may import only these modules; everything else in the project is internal.

| Module | What it gives |
|---|---|
| `Engine.Control.Layer` | The facade: input, action and bot stores, services and detours. Owner only. |
| `Engine.Control.Input.Type`, `Engine.Control.Input.State` | Input sources and states, `VirtualInputStore`, `PlayerActionStore`, `PossessionStore`, `PossessRequestStore` (`ClientDriven`, `PossessStatus`, `AgentDelivery`, `AgentClient`, `AgentClientStatus` and `AgentPlan` are in the Type module), `AgentClientStore`, `ReadAction` (a field of a `PlayerAction` by offset). |
| `Engine.Control.Input.System` | The pure `FindPossessedBot`, `ShouldNeutralizeLocal`, `PrepareFrame`, `AccumulateFrame`, `TakeTickAction`, the pure `AdvanceAgentWindow`, `GetAgentTickInput`, `MakeAgentTickFrame`, `PrepareAgentFrame`, `ResetAgentUnit`, `UndoAgentFrame`, `PlanAgent`, `AgentClientService` (the client per tick of the agents, VW.9c.3 C and D), `ReadAgentClientSettings`, `PossessionService` (the client per frame of the possessed bot) and the client source reads `ReadClientSettings`, `ReadDualWieldRestrictions`, `ReadInputTable` (VW.9c.2 B), the `ActionSource` port, the input services, the pure `FindViewActionPlayer` (which player's action goes into the input of a view) and the pure `ReadInputFrame`, `ReadClientState`, `CompareClient`, `CountClient`, `FormatClientLine` and `FormatClientStateFields` of the client oracle, and the control tag helpers (`LocateControlTag`, `ReadControlTag`, `FindInputPlayer`, `GetEngineThreadContext`). |
| `Engine.Control.Simulation.Type`, `Engine.Control.Simulation.State` | Machines and players of the simulation, the pure `PlanRoster` (bots to move, remove and add to reach a count per team; the `Roster` type comes from `Engine.Base.MatchSetup.Type`), `FindFreeBotNumber` (lowest free bot number), `MakePlayerUpdate` (buffer of the player update), `BotStore` (bot, team and remove requests, session snapshot; bots are added only by a roster, `BotService::AddBot` through `ApplyRoster`, there is no request to add one). `BotService` is the `RosterPort` of `MatchSetupService`. |
| `Engine.Control.Simulation.System` | `BotService` and its partition `:UnitContext`: the `UnitFactPort` (memory and engine calls behind the unit facts of the client) and the pure `BuildUnitContext` (VW.9c.2). |

## Depends on
`Common`, `Services`, `Engine.Base`, `Control`, `Blam.Layout`.

## State between ticks
The overrides and action history of every controlled player, the virtual input of each local player, the bots and their machines.

## Threads
Game thread for the hooks and the engine functions. `ActionSource::Prepare` may block there in Lockstep: at most
`k_LockstepTimeout` per wait, no limit during a pause of a live client, at once when the client leaves. Since TG.1 (D380) the wait starts before the tick,
at the edge between ticks (`TickGate`, `Engine.Capture`), and the wait in `Prepare` only continues it or serves as the fallback when the tick gate is not installed. The stores are thread-safe
for the UI and the bench.

## Errors
An engine function with a changed signature logs an ERROR, never silently. Without `CopyActionToView` the log says "Failed to find
CopyActionToView" and the view of a non local player does not turn; the rest works. Without the player control (rva 0x5DF80 not at the start of a function, or the control tag or thread context not found) the log says "Failed to find the player control" and the client oracle stays off, nothing else changes. Without the player update function (0x418ff8) bots are
still added but never removed. A bot that cannot be removed (the engine refuses a step, the slot stays taken) logs the step and stays
in the `BotStore`; a retry resumes where it stopped. Only the bots Artemis added are removed, never a local or real player. `PlayerActionDetour` without an action source does not
install. A lost client gives its players back to the engine.

## Requirements
Local players with an override and bots receive the same action.

## Tests
`ClientFrameTests` (the table and block bytes the oracle reads, the comparison), `ViewActionTests` (which action goes into the view of a non local player), `EngineLayerTests` (builds without the game; refuses to
install the action hook without a source) and `BotRosterTests` (roster plan,
bot numbers, update buffer, remove requests, what the setup service does on a tick) in `Artemis.Tests`. In game:
`Agent/scripts/bridge_check.py` (`run`, `turn`) and the bots stress scenario of the bench.
