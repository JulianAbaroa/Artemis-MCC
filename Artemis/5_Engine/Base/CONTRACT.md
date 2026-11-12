# Artemis.Engine.Base

Group `5_Engine` - static library inside the game process - the base every other engine project builds on.

## Responsibility
Talking to the game process at the lowest level: detours and patches (MinHook, compiled here), signature (AOB) search, safe memory
reads, the engine's thread-local context, and the lifecycle of the game (engine initialized, map file opened with its path,
subsystems destroyed, the read window of each tick). Also the host side of MCC: the game manager vtable and the local users
Artemis adds next to the real one (split-screen). And the gametype presets (Q97, D264): the compiled files of `Tools/presets.py` are read
from `<storage>/Presets`, the active one is kept in a store and `PresetService` writes it into a copy of the start configuration. The map variant (Q98, D266) and the bots (D284) go the same way. The three are one
match setup (LS.2, D286): `MatchSetupService` takes one request (`preset?`, `map?`, `roster?`) and applies it whole or not at all, at the start of the
match or at a round break, always in the order preset, map variant, roster. `PresetService` and `MapVariantService` only read, plan and write
buffers they are given; they keep no request and no result.

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
| `Engine.Base.Hook.Type`, `Engine.Base.Hook.System` | `InstallDetour`, `RemoveDetour`, `WritePatch`, `InFlightScope`, TLS resolution, `ReadRoundState` and `ReadRoundIndex` (round of the game engine globals, game thread). |
| `Engine.Base.Memory.Type`, `Engine.Base.Memory.System` | `MemoryReaderService` (reads that never fault) and `AOBService` (`FindPattern` for a signature, `FindFunctionAtRva` for a function known only by its RVA and the start of its prologue, R3). |
| `Engine.Base.Lifecycle.Type`, `Engine.Base.Lifecycle.State`, `Engine.Base.Lifecycle.System`, `Engine.Base.Lifecycle.Hook` | Engine status, callbacks, the read window of the tick and the lifecycle detours (initialize, open map, destroy subsystems). |
| `Engine.Base.Host.Type`, `Engine.Base.Host.State`, `Engine.Base.Host.System` | Game manager of MCC (vtable entries), `LocalUserStore` (XUIDs, names, profiles, gamepad mappings), `HostService`. |
| `Engine.Base.MapVariant.Type`, `Engine.Base.MapVariant.State`, `Engine.Base.MapVariant.System` | `ActiveMapVariant` (name of a recorded configuration and the bytes of its map variant block), `MapVariantLivePlan`, `MapVariantStore` (the active map variant and the one the start carried) and `MapVariantService` (`Resolve` makes a map variant out of the block the caller read from the recorded configuration, none gives the map of the load; `Adopt`/`GetActive` keep it; `Check` compares it with the map the start carried; `BeginStart` keeps that map; `WriteToStart` checks it fits and writes it into the start configuration, reads it back and leaves the copy as it was on a failure; `PrepareLive` checks against the runtime map variant without writing and `CommitLive` writes the runtime one, keeping its world bounds, and its copy in the game options, reads both back and restores both on a mismatch). |
| `Engine.Base.Preset.Type`, `Engine.Base.Preset.State`, `Engine.Base.Preset.System` | `PresetPick` (name, compiled preset and catalog, none for the base values), `PresetLivePlan`, `PresetStore` (active preset, catalog, base values saved at the load and their catalog) and `PresetService` (`Resolve` reads `<name>.txt` and `_catalog.txt`; `Adopt`/`GetActive` keep the pick; `Check` compares its catalog with the base of the match; `WriteToStart` writes the preset into a start configuration with the logic of `Blam.Layout`, reads it back and leaves the copy as it was on any failure; `PrepareLive` plans it against a live variant it is given (base saved from the variant at the first preset of the match) and `CommitLive` writes and reads it back, restoring the variant on a mismatch; `AdoptLive` keeps what a committed plan changed; `DropBase` forgets the base of the match that ended). |
| `Engine.Base.MatchSetup.Type`, `Engine.Base.MatchSetup.State`, `Engine.Base.MatchSetup.System` | `MatchSetup` (`Preset?`, `Map?`, `Bots?`: an absent field is no change, preset "" is the base values and map "" the one the match loaded with), `MatchSetupOrigin` (bridge, bench, UI), `MatchSetupResult` (`Pending`, `AtLoad`, `Applied` or `Rejected`, round, reason and the number of the request), `Roster` (the bots per team, here so Control and Base both see it), `k_MaxBots` and `CheckRosterFit` (the one place that decides whether bots fit: `PlanRoster` and the setup both use it), `GetMatchSetupStep`, `ToText`; `MatchSetupStore` (the request waiting with its origin, the result, the active bots and what the start leaves for the first tick; `Cleanup` keeps the active bots and a waiting request); `MatchSetupService` and `RosterPort` (what `Engine.Control` implements and `Mod` connects with `SetRosterPort` so Base can check and apply the bots). Rules: `Request` replaces the waiting request, and with `isMerge` adds its fields to a waiting request of the same origin (the bridge asks for preset and map in two calls); a request that fails its checks, or whose names cannot be resolved (`Reject`), is Rejected but does not clear an older valid request that waited (its own result is dropped as older); `Cancel(origin)` drops only a request of that origin. `BeginStart`/`ApplyToStart` write preset and map variant into the start configuration (restored whole if one fails; a refused setup leaves the start with the setup the engine held before, and a start with no request publishes no result). `UpdateLoad` (first tick) applies the bots of the start and marks a request that waits then as part of the load; `UpdateBreak` (before the ticks of every frame) is the single apply point of a match: it takes the request at a round break (or in the first frame after the load) and checks all of it, then writes preset, map variant and roster in that order. Rejected means nothing was applied: a roster that fails after the preset or the map variant went in is Applied (AtLoad at load) with the reason "partial: ...", there is no rollback of bots. |

## Depends on
`Common`, `Services`, `Blam.Layout` (the pure logic of the presets and the offsets of the game engine globals).

## State between ticks
The engine status and the read window, the addresses found by signature, the local users and their profiles, the active preset with the base values, the active map variant, the active bots and the request waiting with the result of the last one (they stay between matches; `Cleanup` of the match neutralizes the result).

## Threads
- Engine functions and anything that needs the engine's thread context run only on the game thread.
- Memory reads may run on the AI thread inside the read window, while the game waits (at most 100 ms per tick).
- `LifecycleStore` is thread-safe: hooks and the main thread write it; the AI, input and main threads wait on it. At teardown the game thread waits 10 ms for the Artemis tick and runs the Cleanup anyway (it is never held by the AI thread): a tick still running reads `CanPublish` before it publishes and between pipeline stages, and is discarded. `GetTeardownEpoch` goes up with every `TearingDown`, so a thread that slept through the teardown, or a tick that only saw `Destroyed`, still finds its per-map state stale and resets it. The teardowns with timeout and the discarded ticks are counted (game thread and AI thread, atomics) and shown in the teardown INFO.
- Remaining risk (F1 C.7): the cut happens between stages, so a stage that is already running when the teardown starts can still overlap the Cleanup and write a store while it is emptied (the stores are mutex-protected, but a stage may repopulate one after its Cleanup). The full fix is that the AI thread owns the cleanup of its stores, triggered by the epoch; it is in the backlog.
- `MapVariantStore`, `PresetStore` and `MatchSetupStore` are thread-safe and hand out immutable snapshots. `MatchSetupService::Request`, `Reject` and `Cancel` run on the bridge, bench or Workbench thread; the names are resolved there (`Resolve` reads the files), so the game thread never touches the disk. `BeginStart` and `ApplyToStart` run on the MCC thread of the start, `UpdateLoad` on the game thread inside the first tick (`PlayerActionDetour`) and `UpdateBreak` on the game thread before the ticks of every frame (`SimulationTicksDetour`); none of them throws, and an exception after a request was taken ends it as Rejected for its number, with preset and map variant restored. The result of an old request never replaces the one of a newer request.

## Errors
A signature not found or a hook that fails to install logs an ERROR with the reason; nothing fails silently. A setup that cannot be applied is Rejected with its reason (a WARNING; ERROR when a write did not read back equal) and nothing of it is written: at the start the copy of the configuration is restored whole, at a round break nothing is written until every field passed its check. A roster that fails halfway has no rollback: the result is Applied (AtLoad) with the reason "partial: roster k of n operations done". A roster that fits the player table but not the match is only known at the first tick: after the preset and the map variant are in the start the result is AtLoad with a partial reason, never Rejected. A corrupted lifecycle
hook marks the engine destroyed and the main thread installs it again.

## Requirements
No other project reads memory, installs detours or touches the TLS except through this one.

## Tests
`EngineLayerTests` (builds and destroys every engine Layer without the game), `LifecycleStoreTests` (the read window) and `PresetTests` and `MapVariantServiceTests`
(files and the write into a start configuration or a live buffer), `MatchSetupTests` (the store and the start of the service) in `Artemis.Tests`; the rest in game with the bench (`Tools/game.py`).
