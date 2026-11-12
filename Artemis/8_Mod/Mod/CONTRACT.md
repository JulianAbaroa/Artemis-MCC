# Artemis.Mod

Group `8_Mod` - the DLL (`wtsapi32.dll` proxy) - composes every project and runs the threads.

## Responsibility
The composition root. `Mod::Layer` owns every Layer in dependency order, connects the engine lifecycle callbacks and the ports
(`BridgeEngineAdapter` implements `Bridge.Transport`'s `BridgeEngine`, reading the map variant of a named configuration with `MatchConfigService` and resolving the preset and the map by name before it asks `Engine.Base` `MatchSetupService` for a one-field setup (a preset request and a map request of the bridge merge into the setup that waits; `Cancel` drops only a request of the bridge; the bridge keeps one result slot for preset and map until the bridge port is reworked, LS.2 sub-commit 2); it connects `BotService` as the `RosterPort` of the setup service (`WireEngine`) and disconnects it at shutdown; its cleanup calls `MatchSetupStore::Cleanup` (the active setup stays for the next match); `BridgeActionSource` implements `Engine.Control`'s
`ActionSource` over `BridgeService`), builds `EngineAccess` for the development tools, registers the tick observers, installs the
hooks and runs the threads:
- Main: installs and supervises the hooks, reinstalls them when the engine is recreated, drives the shutdown.
- AI: loads the map data, then every tick copies the Snapshot, ends the read window, runs the pipeline, hands decision ticks to the
  bridge and the `ObservedTick` to the observers; times every stage.
- Bridge, Dump, Bench: the bridge transport, the TickDump writer and the in-game bench.
`DllMain` and `Proxy/` forward the real `wtsapi32` exports.

## Does not
- Contain domain logic: it only builds, orders, connects and measures.

## Input and output
- Input: `DllMain` when MCC loads the proxy.
- Output: the whole system running inside the game.

## Public interface
None: it is the root, nothing imports it.

## Depends on
Every other project of the solution except `Tests` and `GoogleTest`.

## State between ticks
Owns every component: built in member order, destroyed in reverse; the threads are the last members, so they are joined first.

## Threads
Creates and stops every thread. The AI thread imports no development tool: the tools are tick observers registered from outside.
Observers must not block.

Sparse mode (Q101): when the bridge clock says IsSparse (the client asked for it, is connected and nobody needs every tick: the
every-tick demand of `SettingsStore` is 0), the AI thread runs a tick that is not a decision as a light tick: `CaptureLight`, `EndRead`
(the game thread is released as in a full tick), `PipelineService::RunLight`. No full pipeline, no Tick, no push to the bridge, no
observer. The first tick after a map load and the first after the mode turned on run the whole pipeline, and so does every decision
tick. A light tick that the teardown cuts is discarded like a full one. Telemetry counts light ticks apart from the sweeps.

## Errors
A pipeline that cannot start (signature not found, hook not installed) is logged with the reason and the UI and the bridge show the
phase. A sweep slow enough to stall the game logs the time of every stage. A tick still running when the engine starts tearing down
(the game thread waits for it only 10 ms and then runs the Cleanup) reads the state again between pipeline stages and before it
publishes: with the engine TearingDown or Destroyed it stops, publishes no Tick, no bridge frame and runs no observer, and is counted
(`LifecycleStore` discarded ticks, logged in the teardown INFO). The AI thread also resets its per-map state when the teardown epoch
changed, even if it never saw TearingDown. This narrows the overlap with the Cleanup, it does not remove it: a stage already running
can still write a store while the Cleanup empties it (backlog: the AI thread owns the cleanup of its stores). The check and the push are
two steps, so observers must stay well under 10 ms (as well as not block).

## Requirements
- Order of the engine callbacks: engine first, then the tools (Gui, Viewer, Workbench), then the pipeline.
- The action source is connected before the action hook is installed.

## Tests
In game, with the bench (`Tools/game.py`: perf and bots stress scenarios, golden film), `Agent/scripts/bridge_check.py` and the
stage-0 smoke test (`Agent/scripts/env_check.py`).
