# Artemis.Tests

Group `7_Tests` - executable - the C++ tests of the components, without MCC.

## Responsibility
`Artemis.Tests.exe`: unit and characterization tests of the projects that run without the game, and of the engine projects as far
as they can be built and destroyed without it. The replay test is the safety net of the whole pipeline: a recorded `.sdump` of a
real match run through `Bridge.Codec.Pipeline.System` must give the approved golden `.tdump` byte for byte. `ReplaySparseTests` replays the films twice, the full pipeline on every record against the sparse mode (pipeline on the
decision records, `MakeLight` and `RunLight` on the others) and compares the Tick field by field with a tolerance list and caps (Q101).

## Does not
- Need the game: anything that needs MCC is the in-game bench (`Tools/game.py`).
- Reference `DevTools.*` or `Mod`, with two exceptions that are compiled into the test executable and are pure (only `import std`). The Viewer Options files (`DevTools.Viewer.Options.Type.ixx`, `OptionsStore.ixx/.cpp`) are tested by `Viewer/OptionsStoreTests.cpp` (save, load, unknown keys, the levels of the layers and the flags they derive). The one source of `Mod` it uses is `TickProducerAdapter` (RV.1): its two files are compiled into the test executable, with
  `Tests.Support.ModPort` as the primary interface of `Mod.Port.System`, so the rules of the game thread run over the real services without MCC.

## Input and output
- Input: hand-made data, the user's Reach `.map` files, recordings (`.sdump` + golden `.tdump`, kept outside the repo).
- Output: one result per test (GoogleTest JSON, summarized by `Tools/test.py`).

## Public interface
None: it is an executable. Its support modules (`Tests.Support.*`) are for its own files. Tests may import internal modules of the
projects they test; the public interface checks of `test_components.py` ignore the imports of this project.

## Depends on
`GoogleTest`, `Common`, `Contract`, `Blam.Layout`, `Blam.Map`, `Blam.MapModel`, `Blam.Runtime`, `Control`, `Perception`, `Views`,
`Bridge.Codec`, `Bridge.Transport`, and the five `Engine.*` projects.

## State between ticks
Not applicable.

## Threads
Tests may start threads to test thread-safe stores; none waits for the game.

## Errors
Missing external data (a `.map`, a recording) or data of another version (size, CRC-32, layout hash) skips the test with its reason
(`GTEST_SKIP`); it never passes silently.

## Requirements
- In each `.cpp`, the GoogleTest `#include`s come before the `import`s.
- Run with `python Tools/test.py` (build through the watcher, then the exe); `--filter` selects suites.
- After an MCC update that changes the layouts: record the film and `.sdump` again and approve them with `Tools/tickdump.py approve`.

## Tests
It is the test. `FlatTests` and `TickDumpWriterTests` cover, since F4.7, the views of the bridge (`ViewsAreCopiedRowByRow`, which also covers `HasFlag`, `ViewMaskAndCountFollowComputedRows`, `RowsOutsideTheSetAreZero`, `EventPositionAndAmountAreZero`) and the version 7 of the `.tdump` (`ViewsAreWrittenAfterLifts`), and `ReplayTests.Recordings` compares the 9 goldens that carry the views block. `ViewsSelfTests` runs the Views of the agents over made-up stores, `ViewRaysTests` their aim hit and rays over made-up planes (the exclusion of each player, the relation of the aim hit, one gather per Update, the dead agent, the raycasts rebuilt from a View), `ViewSightTests`, `ViewOthersTests` and `ViewFlagsTests` who sees whom, what each agent knows of the others and the flags (the made-up objects are in `Tests.Support.ViewScene`), `DISABLED_ViewSightReplayTests` the invariants of those on ctf_c and ctf_d (by hand), `ViewObservedTests` the choice of the watched View, `RaycastViewUpdateTests` the scene through `ViewsService`, `DISABLED_ViewsReplayTests` checks their Self on ctf_c and `DISABLED_ViewsBench` times `ViewsService::Update` (both by hand: `--disabled`). The Python side lives in `Tools/Tests` (`python Tools/test.py --python`): component boundaries
(`test_components.py`), generators and the `.tdump` reader.
