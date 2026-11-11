# Artemis.Engine.Base

Group `5_Engine` - static library inside the game process - the base every other engine project builds on.

## Responsibility
Talking to the game process at the lowest level: detours and patches (MinHook, compiled here), signature (AOB) search, safe memory
reads, the engine's thread-local context, and the lifecycle of the game (engine initialized, map file opened with its path,
subsystems destroyed, the read window of each tick). Also the host side of MCC: the game manager vtable and the local users
Artemis adds next to the real one (split-screen). And the gametype presets (Q97, D264): the compiled files of `Tools/presets.py` are read
from `<storage>/Presets`, the active one is kept in a store and `PresetService` writes it into a copy of the start configuration. A preset requested during a match waits in the store and
`PresetService::ApplyBetweenRounds` writes it into the live variant at a round break (Q97 sub-commit 3).

## Does not
- Capture the tick (`Engine.Capture`), apply actions (`Engine.Control`), launch matches (`Engine.Match`) or draw (`Engine.Render`).
- Install anything or register callbacks by itself: `Mod` connects the lifecycle and installs the hooks.

## Input and output
- Input: the game process.
- Output: lifecycle events and status (`LifecycleStore`), hook and memory services for the other `Engine.*` projects, the local
  users and the game manager functions.

## Public interface
Other projects may import only these modules; everything else in the project is internal.

| Module | What it gives |
|---|---|
| `Engine.Base.Layer` | The facade: lifecycle, host and memory stores and services, and their detours. Owner only. |
| `Engine.Base.Hook.Type`, `Engine.Base.Hook.System` | `InstallDetour`, `RemoveDetour`, `WritePatch`, `InFlightScope`, TLS resolution. |
| `Engine.Base.Memory.Type`, `Engine.Base.Memory.System` | `MemoryReaderService` (reads that never fault) and `AOBService` (`FindPattern` for a signature, `FindFunctionAtRva` for a function known only by its RVA and the start of its prologue, R3). |
| `Engine.Base.Lifecycle.Type`, `Engine.Base.Lifecycle.State`, `Engine.Base.Lifecycle.System`, `Engine.Base.Lifecycle.Hook` | Engine status, callbacks, the read window of the tick and the lifecycle detours (initialize, open map, destroy subsystems). |
| `Engine.Base.Host.Type`, `Engine.Base.Host.State`, `Engine.Base.Host.System` | Game manager of MCC (vtable entries), `LocalUserStore` (XUIDs, names, profiles, gamepad mappings), `HostService`. |
| `Engine.Base.MapVariant.Type`, `Engine.Base.MapVariant.State` | `MapVariantOutcome`, `MapVariantResult`, `ActiveMapVariant` (name of a recorded configuration and the bytes of its map variant block) and `MapVariantStore` (active map variant and result of the last load, Q98, D266). Data only: the file is read by `Engine.Match` (`MapVariantService`) before the block is set. |
| `Engine.Base.Preset.Type`, `Engine.Base.Preset.State`, `Engine.Base.Preset.System` | `PresetOutcome` and `PresetResult`, `PresetStore` (active preset, catalog, base values saved at the load, result of the last load) and `PresetService` (`SetActive` reads `<name>.txt` and `_catalog.txt`, `Request` takes a request of the bridge (the lobby makes it the active preset, a match leaves it pending; when MCC is back in the lobby but its state still says Running the request stays pending and the next start takes it, the same effect), `ApplyBetweenRounds` serves it into a live variant buffer it is given (only with the round ended or between rounds and a game mode with a variant, base saved from the variant at the first request of the match, a refused plan leaves the variant and the active preset as they were), `ApplyToStart` writes the preset into a start configuration with the logic of `Blam.Layout`, reads it back and leaves the copy as it was on any failure). |

## Depends on
`Common`, `Services`, `Blam.Layout` (only the pure logic of the presets).

## State between ticks
The engine status and the read window, the addresses found by signature, the local users and their profiles, the active preset with the base values and the result of the last load.

## Threads
- Engine functions and anything that needs the engine's thread context run only on the game thread.
- Memory reads may run on the AI thread inside the read window, while the game waits (at most 100 ms per tick).
- `LifecycleStore` is thread-safe: hooks and the main thread write it; the AI, input and main threads wait on it. At teardown the game thread waits 10 ms for the Artemis tick and runs the Cleanup anyway (it is never held by the AI thread): a tick still running reads `CanPublish` before it publishes and between pipeline stages, and is discarded. `GetTeardownEpoch` goes up with every `TearingDown`, so a thread that slept through the teardown, or a tick that only saw `Destroyed`, still finds its per-map state stale and resets it. The teardowns with timeout and the discarded ticks are counted (game thread and AI thread, atomics) and shown in the teardown INFO.
- Remaining risk (F1 C.7): the cut happens between stages, so a stage that is already running when the teardown starts can still overlap the Cleanup and write a store while it is emptied (the stores are mutex-protected, but a stage may repopulate one after its Cleanup). The full fix is that the AI thread owns the cleanup of its stores, triggered by the epoch; it is in the backlog.
- `MapVariantStore` is thread-safe and hands out an immutable snapshot of the active map variant; the game thread never reads a disk.
- `PresetStore` is thread-safe and hands out immutable snapshots. `PresetService::ApplyToStart` runs on the MCC thread of the start and never throws. `ApplyBetweenRounds` runs on the game thread inside the tick hook and never throws; `Request` runs on the bridge or bench thread and reads the files there, so the game thread never touches the disk.

## Errors
A signature not found or a hook that fails to install logs an ERROR with the reason; nothing fails silently. A preset that is missing, wrong or that does not read back equal logs an ERROR (a rejected one or a variant that is not Megalo, a WARNING), is left in the result of the store and the start goes on without it. A corrupted lifecycle
hook marks the engine destroyed and the main thread installs it again.

## Requirements
No other project reads memory, installs detours or touches the TLS except through this one.

## Tests
`EngineLayerTests` (builds and destroys every engine Layer without the game), `LifecycleStoreTests` (the read window) and `PresetTests`
(store, files and the write into a start configuration) in `Artemis.Tests`; the rest in game with the bench (`Tools/game.py`).
