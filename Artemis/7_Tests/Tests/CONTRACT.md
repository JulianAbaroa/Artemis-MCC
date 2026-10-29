# Artemis.Tests

Group `7_Tests` - executable - the C++ tests of the components, without MCC.

## Responsibility
`Artemis.Tests.exe`: unit and characterization tests of the projects that run without the game, and of the engine projects as far
as they can be built and destroyed without it. The replay test is the safety net of the whole pipeline: a recorded `.sdump` of a
real match run through `Bridge.Codec.Pipeline.System` must give the approved golden `.tdump` byte for byte.

## Does not
- Need the game: anything that needs MCC is the in-game bench (`Tools/game.py`).
- Reference `DevTools.*` or `Mod`.

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
It is the test. The Python side lives in `Tools/Tests` (`python Tools/test.py --python`): component boundaries
(`test_components.py`), generators and the `.tdump` reader.
