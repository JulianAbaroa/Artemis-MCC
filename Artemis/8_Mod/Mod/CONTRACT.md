# Artemis.Mod

Group `8_Mod` - the DLL (`wtsapi32.dll` proxy) - composes every project and runs the threads.

## Responsibility
The composition root. `Mod::Layer` owns every Layer in dependency order, connects the engine lifecycle callbacks and the ports
(`BridgeEngineAdapter` implements `Bridge.Transport`'s `BridgeEngine`, reading the map variant of a named configuration with `MatchConfigService` and resolving the preset and the map by name before it asks `Engine.Base` `MatchSetupService` for the one setup of a bridge request (`RequestSetup` carries the preset, the map and the roster, so there is no merge; a name that cannot be resolved is `Reject`ed; `Cancel` drops only a request of the bridge; the result, the round and the reason of the setup go to the bridge as one `BridgeSetupInfo`, the reason as ASCII); it connects `BotService` as the `RosterPort` of the setup service (`WireEngine`) and disconnects it at shutdown; its cleanup calls `MatchSetupStore::Cleanup` (the active setup stays for the next match); `BridgeActionSource` implements `Engine.Control`'s
`ActionSource` over `BridgeService`), builds `EngineAccess` for the development tools, registers the tick observers, installs the
hooks and runs the threads:
- Main: installs and supervises the hooks, reinstalls them when the engine is recreated, drives the shutdown.
- AI: loads the map data, then takes the queued ticks in order (the game thread copied each Snapshot, D305), runs the pipeline over them,
  hands decision ticks to the bridge and the `ObservedTick` to the observers; times every stage. It never reads game memory.
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

Ticks (D305): `TickProducerAdapter` (a port, the `TickProducer` of `Engine.Capture`) runs on the game thread from the tick hooks. It reads the bridge
clock (`CaptureClock`: the action tick of the state about to be copied), decides a light or a full capture and copies with `CaptureService`, then
pushes a `CapturedTick` (Snapshot, clock, generation, epoch, light, ticks lost before, first after load) into the `TickQueue` (one producer,
one consumer, FIFO, `k_TickQueueDepth` = 4). It queues only while the AI thread has the queue open (it is consuming: `Running` with the map
resources loaded) and `GetLoadedEpoch` equals the teardown epoch. A queue full for `k_ReadTimeout` loses the tick (counted in the next one as
DroppedBefore, which the AI thread logs; the hook adds it to the dropped ticks of the telemetry when it happens, so a discard never loses it). A decision tick that was queued is marked for the bridge right there (queued for the AI thread, not delivered to the client, RV.1), so
Lockstep waits at the tick of the decision plus the latency for the answer of the client even when the AI thread is behind. The first tick of a
teardown epoch restarts the numbering, the sparse state and the clock of the bridge. Ticks that go by without a clock read (the queue shut or the
map not loaded) make the next read not consecutive, so the jump is not counted as a clock skip (RV.1). The AI thread waits for a queued tick up to 10 ms, then looks
at the lifecycle again.

Sparse mode (Q101): when the bridge clock says IsSparse (the client asked for it, is connected and nobody needs every tick: the
every-tick demand of `SettingsStore` is 0), the game thread captures a tick that is not a decision with `CaptureLight` (D305) and the AI thread runs
`PipelineService::RunLight` over it. No full pipeline, no Tick, no push to the bridge, no
observer. The first tick after a map load and the first after the mode turned on run the whole pipeline, and so does every decision
tick. A light tick that the teardown cuts is discarded like a full one. Telemetry counts light ticks apart from the sweeps.

## Errors
A pipeline that cannot start (signature not found, hook not installed) is logged with the reason and the UI and the bridge show the
phase. A sweep slow enough to stall the game logs the time of every stage. A tick still running when the engine starts tearing down
(the game thread waits for it up to 1 s, C.9, and then runs the Cleanup) reads the state again between pipeline stages and before it
publishes: with the engine TearingDown or Destroyed it stops, publishes no Tick, no bridge frame and runs no observer, and is
counted (`LifecycleStore` discarded ticks, logged in the teardown INFO). When the teardown epoch changed (even if the AI thread never saw
TearingDown) it resets its per-map state, closes the queue and discards the ticks still in it (counted as discarded, one INFO line), and a tick
it takes from the queue with an epoch other than the current one is discarded, never processed against the stores of the new map. The game thread
waits at teardown only for the tick in progress, not for the queued ones. With the stage-boundary cut the stages end within a few milliseconds and
the observers after them must too (they are not cut), so the Cleanup does not overlap the tick unless the AI thread hangs past 1 s.
The check and the push are two steps, so observers must stay well under the 1 s cap (as well as not block).

## Requirements
- Order of the engine callbacks: engine first, then the tools (Gui, Viewer, Workbench), then the pipeline.
- The action source is connected before the action hook is installed.

## Tests
`TickProducerTests` in `Artemis.Tests` (RV.1) run `TickProducerAdapter` over the real `CaptureService` (fake memory), `BridgeService` (store with a fake region),
queue and `LifecycleStore`: the test executable compiles its two sources. In game, with the bench (`Tools/game.py`: perf and bots stress scenarios, golden film), `Agent/scripts/bridge_check.py` and the
stage-0 smoke test (`Agent/scripts/env_check.py`).
