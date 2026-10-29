# Artemis (C++)

The in-process part of Artemis-MCC: a proxy DLL that reads Halo Reach every logic tick, turns what it reads into a Tick, and hands
it to a Python client over shared memory. The code is split into 23 Visual Studio projects. Each one has a `CONTRACT.md` next to
its `.vcxproj`: to work on a project, read its contract and the contracts of what it depends on, not the code of the others.

## Projects
The solution folders of `Artemis-MCC.slnx` mirror the folders on disk: project `Artemis.<Name>` lives in
`Artemis/<Group>/<Folder>/Artemis.<Name>.vcxproj`. No group depends on a later one, and inside a group the alphabetical order is
also an order of dependencies. *Pure* projects never touch the game process, the operating system or a clock: they take data and
return data, so they are tested without MCC.

| Group | Project | Kind | What it is |
|---|---|---|---|
| `1_Shared` | [Common](1_Shared/Common/CONTRACT.md) | pure | Math, geometry, text, hashing, the published value (`Store`), the log port, semantic ids. |
| | [Contract](1_Shared/Contract/CONTRACT.md) | pure, generated | Binary layout of the bridge, shared with Python. |
| | [Services](1_Shared/Services/CONTRACT.md) | disk | Logs, settings, preferences, telemetry. |
| `2_Blam` | [Blam.Layout](2_Blam/Layout/CONTRACT.md) | pure | The engine's memory as data; the Snapshot and its `.sdump` recording. |
| | [Blam.Map](2_Blam/Map/CONTRACT.md) | pure | Reads the `.map` file format. |
| | [Blam.MapModel](2_Blam/MapModel/CONTRACT.md) | pure | What Artemis needs from a map, built once per map. |
| | [Blam.Runtime](2_Blam/Runtime/CONTRACT.md) | pure | Live objects, players, interaction and game state of a tick. |
| `3_Domain` | [Control](3_Domain/Control/CONTRACT.md) | pure | Logic of the player action and of the bridge's lockstep. |
| | [Perception](3_Domain/Perception/CONTRACT.md) | pure | The world as the engine knows it (entities, players, details, aim, events, match). |
| | [Views](3_Domain/Views/CONTRACT.md) | pure | What one agent can know. |
| `4_Bridge` | [Bridge.Codec](4_Bridge/Codec/CONTRACT.md) | pure | Order of the tick, the Tick, its flat form and the `.tdump` recording. |
| | [Bridge.Transport](4_Bridge/Transport/CONTRACT.md) | shared memory | The bridge with the Python client. |
| `5_Engine` | [Engine.Base](5_Engine/Base/CONTRACT.md) | game process | Hooks, memory, lifecycle, local users. |
| | [Engine.Capture](5_Engine/Capture/CONTRACT.md) | game process | Tick hooks and the copy of the Snapshot. |
| | [Engine.Control](5_Engine/Control/CONTRACT.md) | game process | Applies the actions; virtual input; bots. |
| | [Engine.Match](5_Engine/Match/CONTRACT.md) | game process | Match cycle and recorded configurations. |
| | [Engine.Render](5_Engine/Render/CONTRACT.md) | game process | Render and window hooks for the tools. |
| `6_DevTools` | [DevTools.Gui](6_DevTools/Gui/CONTRACT.md) | game process | ImGui backend and widgets. |
| | [DevTools.Viewer](6_DevTools/Viewer/CONTRACT.md) | game process | 3D debug viewer; `EngineAccess`. |
| | [DevTools.Workbench](6_DevTools/Workbench/CONTRACT.md) | game process | Launcher, inspectors, recorders, in-game test bench. |
| `7_Tests` | [GoogleTest](7_Tests/GoogleTest/CONTRACT.md) | third party | GoogleTest and gMock. |
| | [Tests](7_Tests/Tests/CONTRACT.md) | executable | C++ tests without MCC, including the replay of a recorded match. |
| `8_Mod` | [Mod](8_Mod/Mod/CONTRACT.md) | the DLL | Composes everything and runs the threads. |

`External/` holds the git submodules; each library is compiled by its only owner: googletest by `GoogleTest`, zlib by `Blam.Map`,
minhook by `Engine.Base`, imgui by `DevTools.Gui`, stb by `DevTools.Workbench`.

## One tick
1. Game thread: the tick hooks of `Engine.Capture` open the read window and the game waits.
2. AI thread (`Mod`): `Engine.Capture` copies the live objects and memory regions into a Snapshot, then the read window ends and
   the game goes on.
3. AI thread: `Bridge.Codec.Pipeline.System` runs the pure chain over the Snapshot: `Blam.Runtime` (tables), `Perception` (the
   world), `Views` (the agent), then the assembly of the Tick.
4. On a decision tick with a client connected, the Tick goes to the bridge thread, which flattens it (`Bridge.Codec`) and publishes
   it (`Bridge.Transport`). Every tick, the `ObservedTick` goes to the observers (viewer, recorders), which must not block.
5. Game thread, next ticks: the PlayerAction hook of `Engine.Control` asks its `ActionSource` (the bridge, connected by `Mod`) for
   the overrides of the client's players, builds each action with `Control` and writes it. In Lockstep it waits for the decision.

## Rules
- A module is named `<Project>.<Specialization>.<Kind>[:Partition]`, and its namespace follows it word by word
  (`Perception.Aim.Type` / `Perception::Aim::Type`). Kinds: `Type` (data), `State` (stores), `System` (logic), `Hook` (detours),
  and one `<Project>.Layer` per project that owns its stores and services.
- A project imports only the projects it references, and only the modules listed under *Public interface* in their contracts.
  `Tools/Tests/test_components.py` checks both, and that every listed module exists. The tests may import internal modules.
- Generated files (`Contract`, datum layouts, tag structs, semantic ids) are never edited by hand: change the source under `Data/`
  or the generator, and regenerate.

## Build and test
- Build: open `Artemis-MCC.slnx` in Visual Studio 2026, `Release|x64`. Tools: `python Tools/build.py` builds through
  `Tools\build_watcher.bat`.
- C++ tests: `python Tools/test.py` (builds and runs `Artemis.Tests`). Python tests: `python Tools/test.py --python`.
- Mechanical code rules: `python Tools/check_rules.py`.
