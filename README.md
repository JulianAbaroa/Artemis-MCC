# Artemis-MCC
Artemis-MCC is an AI training framework for Halo: The Master Chief Collection, targeting Halo Reach on Windows (Steam). The project's goal is to train a reinforcement learning agent to play Halo Reach by extracting real-time game state data directly from the engine.
The project is in active development. The data extraction layer works, the semantic layers that turn raw engine data into an observation for the agent are being built, and the Python side has a first Capture the Flag environment and a PPO trainer that run end to end against the game.

## How it works
Artemis operates as a proxy DLL (`wtsapi32.dll`), injecting into the game process via auto-proxy injection. Once inside, it uses function hooking to read game state data in real-time from the engine, player table, object table, read .map files, and other relevant data extracted directly from memory and disk.

Artemis works in engine logic ticks (60 per second of game time), never in milliseconds. It hooks the functions that run the simulation so it sees every tick, and the game waits for Artemis to copy the data of each tick before it writes the next one. Because of that the data stays consistent at any game speed, and no tick is ever skipped: if Artemis is slower than the game, the game slows down. See [`docs/Tick Synchronization.md`](docs/Tick%20Synchronization.md).

Training needs the game to run faster than real time. Halo Reach simulates with a fixed time step, and the engine has its own time scale (the same mechanism Theater uses to fast-forward films), so the game can run many times faster without changing the length of a tick or the physics.

The project has three parts:
- **Artemis (C++, `Artemis/`):** The in-process module. Hooks into the game engine, copies each tick into a snapshot, turns it into a Tick for the agent, and serves it to Python over shared memory. It is split into 23 projects, each with a public contract; start at [`Artemis/README.md`](Artemis/README.md).
- **Agent (Python, `Agent/`):** The client of the bridge: the environment the agent sees and the training code.
- **Tools (Python, `Tools/`):** `TagTranspiler/` parses Halo Reach tag definition files to generate the C++ headers used by Artemis to read tag structures from `.map` files. The other scripts analyze the captures and dumps that Artemis writes during reverse engineering.

## Repository layout
- `Artemis/`: the C++ proxy DLL in 23 Visual Studio projects grouped in numbered folders (submodules in `External/`). Each project has a `CONTRACT.md`; the index is [`Artemis/README.md`](Artemis/README.md).
- `Agent/`: the Python client of the bridge (environment, training, scripts).
- `Tools/`: development tools in Python (`TagTranspiler/` generators, seed scripts, capture and dump analysis). Not needed at runtime.
- `Data/`: hand-edited XML that is the source of truth for the generated code (datum layouts, tag catalog, semantic ids). See `Data/README.md`.
- `docs/`: technical notes.

`Artemis-MCC.slnx` holds the C++ projects, in solution folders that mirror the folders on disk, and one Python project (`Artemis-MCC.pyproj`) that shows `Tools/` and `Data/`.

## Technical Requirements
- **Language:** C++23
- **Compiler:** Visual Studio 2026
- **Platform:** Windows 10/11 (x64)
- **Dependencies:**
  - [MinHook](https://github.com/TsudaKageyu/minhook), BSD-2-Clause
  - [Dear ImGui](https://github.com/ocornut/imgui), MIT
  - [zlib](https://zlib.net), zlib License
  - [stb_image](https://github.com/nothings/stb), Public Domain / MIT
  - [GoogleTest](https://github.com/google/googletest) (with gMock), BSD-3-Clause. Only for the test executable, not linked into the DLL.

## Building

### Prerequisites
- **Visual Studio 2026** with the C++ Desktop Development workload (C++23 support).
- **Python 3.10+** (for the tools in `Tools/`).
- A copy of **[Assembly](https://github.com/XboxChaos/Assembly)**, its Halo Reach MCC plugin XML files are required to generate the tag headers (see step 2). These files are **not** included in this repository.
- **Halo: The Master Chief Collection** (Steam), with Halo Reach installed and **Easy Anti-Cheat (EAC) disabled**.

### 1. Clone with submodules
Artemis depends on MinHook, Dear ImGui, zlib and GoogleTest as git submodules, so clone recursively:

```bash
git clone --recursive https://github.com/JulianAbaroa/Artemis-MCC.git
```

If you already cloned without `--recursive`:

```bash
git submodule update --init --recursive
```

### 2. Build Artemis
Open `Artemis-MCC.slnx` in Visual Studio 2026, select the **x64** and **Release** configurations, and build the solution. This produces the proxy DLL (`wtsapi32.dll`, project `Artemis.Mod`) and the test executable (`Artemis.Tests`).

### 3. Run
Artemis injects into the Halo Reach MCC process via DLL proxy injection. Place the built `wtsapi32.dll` next to the game's executable (C:\path\to\MCC\Binaries\Win64) so it loads in place of the system library, then launch the game (with EAC disabled).

## Testing
The tests run without the game:
- **C++:** `python Tools/test.py` builds and runs `Artemis.Tests` (GoogleTest): unit tests of the components and a replay of a recorded match through the whole pipeline, compared byte for byte with an approved recording. Tests that need data not in the repo (the Reach `.map` files, the recordings) are skipped with their reason when it is missing.
- **Python:** `python Tools/test.py --python` runs the pytest suite in `Tools/Tests/`, which also checks the boundaries between the C++ projects and their contracts.

Everything that needs the game runs on the in-game test bench (`Tools/game.py`).

## Credits and Acknowledgments
- **[Assembly](https://github.com/XboxChaos/Assembly):** Tag definition plugin XML files, consumed by TagTranspiler to generate the C++ tag structure headers used by Artemis. Licensed under GPL-3.0.
- **[Reclaimer](https://github.com/Gravemind2401/Reclaimer):** Reference implementation for Halo Reach MCC geometry extraction. The mesh-reading logic of the map geometry reader was derived/ported from Reclaimer's C# code (see `docs/MeshReader.md`). Licensed under GPL-3.0.
- **[AlphaRing](https://github.com/WinterSquire/AlphaRing):** Conceptual DLL proxy injection approach using wtsapi32.dll, and the split-screen technique Artemis uses for its local players: reporting extra local users and per-player input through the game manager interface MCC gives the game, plus the two network session changes that let those users join. Studied as a reference; no code from it is used.
- **[libmcc](https://github.com/SpringContingency/libmcc):** Reference for the MCC game manager interface (vtable layout, input state structure). No code from it is used.
- **[HaloCheckpointManager](https://github.com/Burnt-o/HaloCheckpointManager):** Studied as a reference while researching how to run the game faster than real time. No code from it is used.
- **[ManagedDonkey](https://github.com/twist84/ManagedDonkey):** Reconstruction of the Halo Online engine, a sibling of the Reach engine. Studied as a reference for the names and roles of the network session, the simulation watcher and the player and machine updates while researching more players per instance. No code from it is used.
- **[Mjolnir-Forge-Editor](https://github.com/Waffle1434/Mjolnir-Forge-Editor):** The author's inspiration and entry point into Halo modding at the game-engine level.
- **[Font Awesome Free](https://fontawesome.com):** UI icons (cube, person-rifle, gear, file-lines, map). Licensed under CC BY 4.0. Icons were downloaded as PNG and embedded as C++ byte arrays via `GenerateIconData.py`.
- **Halo Modding Community:** Contributions to the understanding of the Blam! engine.

## License
Artemis-MCC is licensed under the GNU General Public License v3.0.
See the [LICENSE](LICENSE) file for details, and [NOTICE](NOTICE) for third-party attributions.

## Disclaimer
**This project is intended for educational and research purposes.** Artemis-MCC is designed to work exclusively with **Easy Anti-Cheat (EAC) disabled**. The developer does not condone or support the use of this tool in any manner that violates software terms of service.

*Note: Artemis-MCC is in early development and may cause unexpected game crashes.*

**Legal:** Halo: The Master Chief Collection © Microsoft Corporation. Artemis-MCC was created under Microsoft's [Game Content Usage Rules](https://www.xbox.com/en-US/developers/rules) using game content and file formats from Halo: Reach, and is not endorsed by or affiliated with Microsoft.
