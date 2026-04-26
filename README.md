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
  - [MinHook](https://github.com/TsudaKageyu/minhook)
  - [Dear ImGui](https://github.com/ocornut/imgui)

## Credits and Acknowledgments

- **[Assembly](https://github.com/XboxChaos/Assembly):** Tag definition files used as reference for generating Artemis's memory layout headers. Licensed under GPL-3.0.
- **[AlphaRing](https://github.com/WinterSquire/AlphaRing):** Foundational DLL proxy architecture and export definitions.
- **[Mjolnir-Forge-Editor](https://github.com/Waffle1434/Mjolnir-Forge-Editor):** Initial inspiration for Halo engine memory manipulation.
- **MinHook:** Function hooking library.
- **Dear ImGui:** Immediate mode GUI library.
- **Halo Modding Community:** Contributions to the understanding of the Blam! engine.

## Disclaimer

**This project is intended for educational and research purposes.** Artemis-MCC is designed to work exclusively with **Easy Anti-Cheat (EAC) disabled**. The developer does not condone or support the use of this tool in any manner that violates software terms of service.

*Note: Artemis-MCC is in early development and may cause unexpected game crashes.*

**Legal:** Halo: The Master Chief Collection © Microsoft Corporation. Artemis-MCC was created under Microsoft's [Game Content Usage Rules](https://www.xbox.com/en-US/developers/rules) using assets and code structures from Halo: Reach, and is not endorsed by or affiliated with Microsoft.
