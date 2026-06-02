# Artemis-MCC
Artemis-MCC is an AI training framework for Halo: The Master Chief Collection, targeting Halo Reach on Windows (Steam). The project's goal is to train a reinforcement learning agent to play Halo Reach by extracting real-time game state data directly from the engine.
The project is in its early stages, currently focused on the data extraction layer.

## How it works
Artemis operates as a proxy DLL (`wtsapi32.dll`), injecting into the game process via auto-proxy injection. Once inside, it uses function hooking to read game state data in real-time from the engine — player table, object table, read .map files, and other relevant data extracted directly from memory and disk.

The data pipeline has two components:
- **Artemis (C++):** The in-process module. Hooks into the game engine, reads tag and runtime data from memory, and exposes it for downstream use.
- **TagTranspiler (Python):** A code generation tool that parses Halo Reach tag definition files to automatically generate the C++ headers used by Artemis to read tag structures from `.map` files.

## Technical Requirements
- **Language:** C++23
- **Compiler:** Visual Studio 2026
- **Platform:** Windows 10/11 (x64)
- **Dependencies:**
  - [MinHook](https://github.com/TsudaKageyu/minhook) — BSD-2-Clause
  - [Dear ImGui](https://github.com/ocornut/imgui) — MIT
  - [zlib](https://zlib.net) — zlib License
  - [stb_image](https://github.com/nothings/stb) — Public Domain / MIT

## Building

### Prerequisites
- **Visual Studio 2026** with the C++ Desktop Development workload (C++23 support).
- **Python 3.10+** (for TagTranspiler).
- A copy of **[Assembly](https://github.com/XboxChaos/Assembly)** — its Halo Reach MCC plugin XML files are required to generate the tag headers (see step 2). These files are **not** included in this repository.
- **Halo: The Master Chief Collection** (Steam), with Halo Reach installed and **Easy Anti-Cheat (EAC) disabled**.

### 1. Clone with submodules
Artemis depends on MinHook, Dear ImGui and zlib as git submodules, so clone recursively:

```bash
git clone --recursive https://github.com/JulianAbaroa/Artemis-MCC.git
```

If you already cloned without `--recursive`:

```bash
git submodule update --init --recursive
```

### 2. Generate the tag headers (required)
The C++ tag structure headers are generated from Assembly's plugin XML files and are **not** committed to this repo, so you must generate them before building. Point TagTranspiler at your Assembly Halo Reach MCC plugin directory:

```bash
cd TagTranspiler
python GenerateTags.py "C:\path\to\Assembly\src\Assembly\Plugins\ReachMCC"
```

This populates `TagTranspiler/Generated/`, which Artemis includes at compile time. The build will fail without these headers.

### 3. Build Artemis
Open `Artemis-MCC.sln` in Visual Studio 2026, select the **x64** configuration, and build the solution. This produces the proxy DLL (`wtsapi32.dll`).

### 4. Run
Artemis injects into the Halo Reach MCC process via DLL proxy injection. Place the built `wtsapi32.dll` next to the game's executable (C:\path\to\MCC\Binaries\Win64) so it loads in place of the system library, then launch the game (with EAC disabled).

## Credits and Acknowledgments
- **[Assembly](https://github.com/XboxChaos/Assembly):** Tag definition plugin XML files, consumed by TagTranspiler to generate the C++ tag structure headers used by Artemis. Licensed under GPL-3.0.
- **[Reclaimer](https://github.com/Gravemind2401/Reclaimer):** Reference implementation for Halo Reach MCC geometry extraction. The mesh-reading logic in System_MeshReader was derived/ported from Reclaimer's C# code. Licensed under GPL-3.0.
- **[AlphaRing](https://github.com/WinterSquire/AlphaRing):** Conceptual inspiration for the DLL proxy injection approach using wtsapi32.dll.
- **[Mjolnir-Forge-Editor](https://github.com/Waffle1434/Mjolnir-Forge-Editor):** The author's entry point into Halo modding at the game-engine level. No code was taken from this project.
- **[Font Awesome Free](https://fontawesome.com):** UI icons (cube, person-rifle, gear, square-binary, file-lines, map). Licensed under CC BY 4.0. Icons were downloaded as PNG and embedded as C++ byte arrays via `GenerateIconData.py`.
- **Halo Modding Community:** Contributions to the understanding of the Blam! engine.

## License
Artemis-MCC is licensed under the GNU General Public License v3.0.
See the [LICENSE](LICENSE) file for details, and [NOTICE](NOTICE) for third-party attributions.

## Disclaimer
**This project is intended for educational and research purposes.** Artemis-MCC is designed to work exclusively with **Easy Anti-Cheat (EAC) disabled**. The developer does not condone or support the use of this tool in any manner that violates software terms of service.

*Note: Artemis-MCC is in early development and may cause unexpected game crashes.*

**Legal:** Halo: The Master Chief Collection © Microsoft Corporation. Artemis-MCC was created under Microsoft's [Game Content Usage Rules](https://www.xbox.com/en-US/developers/rules) using game content and file formats from Halo: Reach, and is not endorsed by or affiliated with Microsoft.
