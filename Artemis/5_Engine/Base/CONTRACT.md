# Artemis.Engine.Base

Group `5_Engine` - static library inside the game process - the base every other engine project builds on.

## Responsibility
Talking to the game process at the lowest level: detours and patches (MinHook, compiled here), signature (AOB) search, safe memory
reads, the engine's thread-local context, and the lifecycle of the game (engine initialized, map file opened with its path,
subsystems destroyed, the read window of each tick). Also the host side of MCC: the game manager vtable and the local users
Artemis adds next to the real one (split-screen).

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
| `Engine.Base.Memory.Type`, `Engine.Base.Memory.System` | `MemoryReaderService` (reads that never fault) and `AOBService`. |
| `Engine.Base.Lifecycle.Type`, `Engine.Base.Lifecycle.State`, `Engine.Base.Lifecycle.System`, `Engine.Base.Lifecycle.Hook` | Engine status, callbacks, the read window of the tick and the lifecycle detours (initialize, open map, destroy subsystems). |
| `Engine.Base.Host.Type`, `Engine.Base.Host.State`, `Engine.Base.Host.System` | Game manager of MCC (vtable entries), `LocalUserStore` (XUIDs, names, profiles, gamepad mappings), `HostService`. |

## Depends on
`Common`, `Services`.

## State between ticks
The engine status and the read window, the addresses found by signature, the local users and their profiles.

## Threads
- Engine functions and anything that needs the engine's thread context run only on the game thread.
- Memory reads may run on the AI thread inside the read window, while the game waits (at most 100 ms per tick).
- `LifecycleStore` is thread-safe: hooks and the main thread write it; the AI, input and main threads wait on it.

## Errors
A signature not found or a hook that fails to install logs an ERROR with the reason; nothing fails silently. A corrupted lifecycle
hook marks the engine destroyed and the main thread installs it again.

## Requirements
No other project reads memory, installs detours or touches the TLS except through this one.

## Tests
`EngineLayerTests` (builds and destroys every engine Layer without the game) and `LifecycleStoreTests` (the read window) in
`Artemis.Tests`; the rest in game with the bench (`Tools/game.py`).
