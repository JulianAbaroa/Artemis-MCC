# Artemis.Mod

Group `8_Mod` - the DLL (`wtsapi32.dll` proxy) - composes every project and runs the threads.

## Responsibility
The composition root. `Mod::Layer` owns every Layer in dependency order, connects the engine lifecycle callbacks and the ports
(`BridgeEngineAdapter` implements `Bridge.Transport`'s `BridgeEngine`; `BridgeActionSource` implements `Engine.Control`'s
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

## Errors
A pipeline that cannot start (signature not found, hook not installed) is logged with the reason and the UI and the bridge show the
phase. A sweep slow enough to stall the game logs the time of every stage.

## Requirements
- Order of the engine callbacks: engine first, then the tools (Gui, Viewer, Workbench), then the pipeline.
- The action source is connected before the action hook is installed.

## Tests
In game, with the bench (`Tools/game.py`: perf and bots stress scenarios, golden film), `Agent/scripts/bridge_check.py` and the
stage-0 smoke test (`Agent/scripts/env_check.py`).
