# Artemis.Common

Group `1_Shared` - pure static library - the vocabulary every other project shares.

## Responsibility
Small, game-agnostic building blocks: vectors and matrices (also the packed engine vectors inside the generated structs), geometry,
reflection, teams, zone shapes, text, hashing, byte streams, the published value (`Store`/`Snapshot`), the log port, the generic
engine sentinels (null handle, ticks per second) and the semantic ids generated from `Data/Semantic/Semantic.xml`.

## Does not
- Know anything of Blam memory or `.map` files: that is the `Blam.*` projects.
- Write files or decide where logs go: it only declares `LogSink`; the sink of the DLL is in `Services`.
- Call the operating system: it builds in the test executable without `windows.h`.

## Input and output
Types, constants and free functions. No global state.

## Public interface
Other projects may import only these modules; everything else in the project is internal.

| Module | What it gives |
|---|---|
| `Common.Engine.Type` | Sentinels and limits shared with the engine: `k_NoHandle`, `k_NoTick`, handle masks, `k_MaxPlayers`, `k_MaxLocalUsers`, `k_MaxObjects`, `k_TicksPerSecond`. |
| `Common.Math.Type`, `Common.Math.System` | `Vec2/3/4`, `Matrix`, `Triangle` (fixed layout, used inside generated structs) and vector math. |
| `Common.Geometry.System` | Ray against triangle. |
| `Common.State.Type` | `Store<T>`: publish an immutable value, readers acquire a `Snapshot` without locks. |
| `Common.Log.Type`, `Common.Log.System` | `LogSink` port, `Log` front end (`Message(fmt, args...)`), parsing of `"[Tag] LEVEL: text"`. |
| `Common.Reflect.Type`, `Common.Reflect.System` | Field reflection and flattening to rows (inspectors). |
| `Common.Semantic.Type` | Semantic ids (kind, function, group, capabilities, events...). Generated; never edited by hand. |
| `Common.Tag.Type` | Tag names of the playable bipeds, armor abilities, pickups and powerups. |
| `Common.Team.Type`, `Common.ZoneShape.Type` | Team and zone shape enums with the game's values. |
| `Common.Text.System` | `WideToUtf8`, JSON escaping, text copies. |
| `Common.Hash.System` | CRC-32 (same as zlib) of bytes and files. |
| `Common.Stream.System` | Raw reads and writes of values and rows on a stream. |
| `Common.Time.System` | Local time for file names and log lines. |

## Depends on
Nothing but the standard library.

## State between ticks
None.

## Threads
Everything is safe to call from any thread, or says otherwise in its comment. `Store` publishes and acquires without blocking.

## Errors
No exceptions. Functions that may have no result return `std::optional` or `false`.

## Requirements
- No operating system headers.
- `Vec2/3/4` keep their packed layout: generated datum and tag structs embed them.

## Tests
`MathTypeTests`, `SnapshotTests` (the `Store`), `LogTests`, `LogParseTests`, `TextTests` (against `WideCharToMultiByte`) and
`HashTests`, in `Artemis.Tests`.
