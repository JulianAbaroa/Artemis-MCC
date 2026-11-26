export module Engine.Base.Memory.Type:Signature;

import std;

export namespace Engine::Base::Memory::Type::Signature
{
    // Byte pattern of a game function. "??" matches any byte.
    struct Signature
    {
        const char* Name{};
        const char* Pattern{};
    };

    // Functions: Lifecycle
    inline constexpr Signature k_EngineInitialize
    {
        "EngineInitialize",
        "48 8B C4 55 41 54 41 55 41 56 41 57 48 8D 68 A1 48 81 EC C0 00 00 00 48 C7 45 B7 FE FF FF FF",
    };

    inline constexpr Signature k_DestroySubsystems
    {
        "DestroySubsystems",
        "40 56 57 41 57 48 83 EC 40 48 C7 44 24 20 FE FF FF FF 48 89 5C 24 68 48 89 6C 24 70 33 F6",
    };

    // MCC-Win64-Shipping.exe: picks the module of a game and creates its engine with the CreateGameEngine export (A15).
    inline constexpr Signature k_LoadGameEngine
    {
        "LoadGameEngine",
        "48 89 5C 24 10 57 48 83 EC 30 48 8B 05 ?? ?? ?? ?? 48 33 C4 48 89 44 24 28 48 8B D9 48 63 FA 48 8B 0D ?? ?? ?? ?? 48 85 C9 74 ?? 39 BB EC BF 02 00"
    };

    // MCC-Win64-Shipping.exe: updates the lifecycle manager of the online layer, once per frame on the MCC thread (D124).
    inline constexpr Signature k_LifecycleUpdate
    {
        "LifecycleUpdate",
        "48 8B C4 57 48 83 EC 70 48 C7 40 B8 FE FF FF FF 48 89 58 10 48 89 70 18 48 8B F9 48 8D B1 C8 01 00 00 48 8B CE E8 ?? ?? ?? ?? 8A 87 98 00 00 00 84 C0 74 ?? C6 87 98 00 00 00 00"
    };

    // MCC-Win64-Shipping.exe: StartCountdown, called by the START of the lobby (D124). Artemis only reads two calls from it: the
    // session getter and the check of a running countdown.
    inline constexpr Signature k_StartCountdown
    {
        "StartCountdown",
        "48 89 5C 24 08 48 89 74 24 10 57 48 83 EC 20 40 8A F1 33 FF FF 15 ?? ?? ?? ?? 84 C0 74 ?? 8D 4F 01 E8 ?? ?? ?? ?? 48 8B C8 E8 ?? ?? ?? ?? 8D 4F"
    };

    // MCC-Win64-Shipping.exe: the check that the map variant of the lobby supports its game variant, called by the START of the
    // lobby (rva 0x43e768 on MCC 1.3528).
    inline constexpr Signature k_VariantsCompatible
    {
        "VariantsCompatible",
        "48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 57 48 83 EC 20 45 8B D9 8B EA 33 FF E8 ?? ?? ?? ?? 8B D5 45 8B C3 48 8B F0 E8"
    };

    // MCC-Win64-Shipping.exe: inside the StartGame state, finds the custom games lobby screen by name in the UI manager (D124).
    // note: Not a function start. The UI manager global, the screen name and the FindScreen function are read from its operands.
    inline constexpr Signature k_FindLobbyScreen
    {
        "FindLobbyScreen",
        "48 8B 0D ?? ?? ?? ?? 48 8D 15 ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 8B C8 48 85 C0 75 ?? 48 8B 0D ?? ?? ?? ?? 48 8D 15 ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 8B C8 48 85 C0 74 ?? 48 8B 01 FF 90 10 01 00 00"
    };

    // Functions: Map
    inline constexpr Signature k_BlamOpenMap
    {
        "BlamOpenMap",
        "48 89 5C 24 08 55 56 57 41 54 41 55 41 56 41 57 48 8D AC 24 80 FD FF FF 48 81 EC 80 03 00 00 48 8B 05 ?? ?? ?? ?? 48 33 C4 48 89 85 70 02 00 00 33 FF"
    };

    // Functions: Objects
    inline constexpr Signature k_TelemetryIdModifier
    {
        "TelemetryIdModifier",
        "8B 0D ?? ?? ?? ?? 65 48 8B 04 25 58 00 00 00 48 8B 04 C8",
    };

    // Functions: Ticks
    inline constexpr Signature k_SimulationTicks
    {
        "SimulationTicks",
        "48 8B C4 48 89 58 08 48 89 70 10 48 89 78 18 4C 89 70 20 41 57 48 83 EC 20 8B 05 ? ? ? ? 4C 8B F2 65 48 8B 04 25 58 00 00 00 8B F9 44 8B 05 ? ? ? ? 41 BF 48 00 00 00"
    };

    // Offset of the call to the pause predicate of the engine inside SimulationTicks (haloreach.dll MD5 012f4079af9a8bb0f624c178af8e0753,
    // 2026-11-25): at the start of every tick of its loop, bytes E8 rel32, so the return address is the offset + 5 (0x6B) (TG.1, D380).
    // note: Five other places call the same predicate, so the hook tells this call by the return address.
    inline constexpr std::uintptr_t k_SimulationTicksPauseCallOffset{ 0x66 };

    // First byte of a near call with a 32 bit displacement.
    inline constexpr std::uint8_t k_CallOpcode{ 0xE8 };

    // Size of the call instruction at k_SimulationTicksPauseCallOffset: E8 and the rel32.
    inline constexpr std::uintptr_t k_SimulationTicksPauseCallSize{ 5 };

    // First 18 bytes of the pause predicate of the engine (FUN_1800356ac, rva 0x356ac, 279 bytes, same haloreach.dll). Exact, without
    // wildcards, and without RIP-relative operands: TickGateDetour installs only if the call target starts with them (TG.1, D380).
    inline constexpr std::array<std::uint8_t, 18> k_PauseCheckPrologue
    {
        0x48, 0x89, 0x5C, 0x24, 0x08, 0x48, 0x89, 0x74, 0x24, 0x10, 0x57, 0x48, 0x83, 0xEC, 0x20, 0x40, 0x32, 0xFF
    };

    // Ends the round of the game engine (0x7b3ec), the call of the end_round action of Megalo (opcode 21, D144).
    inline constexpr Signature k_EndRound
    {
        "EndRound",
        "48 89 5C 24 08 48 89 74 24 10 48 89 7C 24 18 55 41 54 41 55 41 56 41 57 48 8B EC 48 83 EC 50 45 8B F8 8B D9 40 8A FA 45 33 E4 E8 ?? ?? ?? ?? 44 8B 2D ?? ?? ?? ?? 4C 8B F0 BA 20 00 00 00"
    };

    inline constexpr Signature k_ExecuteGameFrame
    {
        "ExecuteGameFrame",
        "48 8B C4 48 89 58 08 48 89 70 10 48 89 78 18 4C 89 60 20 55 41 56 41 57 48 8D A8 28 29 FF FF B8 C0 D7 00 00 E8 ?? ?? ?? ?? 48 2B E0 48 8B 05 ?? ?? ?? ?? 48 33 C4 48 89 85 B8 D6 00 00"
    };

    // Applies the player actions of one tick: void(simulation update). ExecuteGameFrame calls it (A9_SplitScreen.txt sec. 12.4).
    inline constexpr Signature k_ApplyPlayerActions
    {
        "ApplyPlayerActions",
        "48 8B C4 55 41 54 41 55 41 56 41 57 48 8D A8 38 FF FF FF 48 81 EC A0 01 00 00 48 C7 44 24 78 FE FF FF FF 48 89 58 10 48 89 70 18 48 89 78 20 0F 29 70 C8 0F 29 78 B8 48 8B 05 ?? ?? ?? ?? 48 33 C4 48 89 45 70 48 8B F9 48 89 4C 24 70 8B 15 ?? ?? ?? ?? 65 48 8B 04 25 58 00 00 00 4C 8B 34 D0 B8 30 01 00 00"
    };

    // Clears a player action to the empty action of the engine: void(action). Every action the engine builds starts with it (0x628b0)
    // (A17 sec. 2, D119).
    inline constexpr Signature k_ClearPlayerAction
    {
        "ClearPlayerAction",
        "48 89 5C 24 08 57 48 83 EC 20 48 8D 59 7B 33 D2 41 B8 A8 00 00 00 48 83 E3 FC 48 8B F9 E8 ?? ?? ?? ?? 41 83 C8 FF C6 47 04 FF 66 44 89 47 44"
    };

    // Copies the action of a player into the input of a view: void(view, action) (haloreach.dll rva 0x62fcc, A21 sec. 9.2).
    // ApplyPlayerActions calls it for the view of the player it shows, but only in the theater (game globals +0x1DA). The four bytes of the
    // index of the thread local storage (rip-relative) are wildcards (VW.9a.y, D349). Unique in the live haloreach.dll of 2026-11-20 (1 hit).
    // note: Artemis calls it after the original 0x47050: the update is not written by it, so the action is the validated one (0x46ac8).
    inline constexpr Signature k_CopyActionToView
    {
        "CopyActionToView",
        "48 89 5C 24 08 57 48 83 EC 20 44 8B 05 ?? ?? ?? ?? 48 8B DA 65 48 8B 04 25 58 00 00 00 BA D8 00 00 00 4E 8B 0C C0 48 63 C1 48 69 C8 B0 00 00 00"
    };

    // Place in 0x5df80 (player control of a local player) that loads the player control tag of the globals: mov rcx, [table]
    // (rel32 at +3), add rcx, 0x418, then call the tag address resolver (0x15918, rel32 at +0x30). Its field +0x48 is the hold
    // threshold of buttons in seconds (A17 sec. 8, D121).
    inline constexpr Signature k_PlayerControlTagLoad
    {
        "PlayerControlTagLoad",
        "48 8B 0D ?? ?? ?? ?? 48 63 FA 48 81 C1 18 04 00 00 F3 44 0F 11 64 24 50 F3 44 0F 11 4C 24 60 89 7C 24 4C 89 5C 24 54 4C 89 65 18 48 89 45 00 E8 ?? ?? ?? ??"
    };

    // Functions: High-level Data
    inline constexpr Signature k_BuildGameEvent
    {
        "BuildGameEvent",
        "48 8B C4 48 89 58 08 48 89 68 10 48 89 70 18 44 89 48 20 57 41 54 41 55 41 56 41 57 48 83 EC 40 4C 8B B4 24 90 00 00 00",
    };

    // Functions: Host
    // Constructor of the game manager in MCC-Win64-Shipping.exe: lea rax, [vtable], mov [game manager], rax, then it clears the
    // first members (xor eax, eax, mov [+0x8], rax, mov [+0x10], eax, and [+0x18], rax, and [+0x20], rax).
    inline constexpr Signature k_GameManager
    {
        "GameManager",
        "48 8D 05 ?? ?? ?? ?? 48 89 05 ?? ?? ?? ?? 33 C0 48 89 05 ?? ?? ?? ?? 89 05 ?? ?? ?? ?? 48 21 05 ?? ?? ?? ?? 48 21 05"
    };

    inline constexpr Signature k_CanAcceptAnyJoinRequest
    {
        "CanAcceptAnyJoinRequest",
        "40 53 48 83 EC 20 33 DB F6 81 00 49 00 00 01 74 4D 48 81 C1 80 48 00 00"
    };

    // Session state check: mov ecx, [state], lea eax, [rcx - 6], cmp eax, 3, ja.
    inline constexpr Signature k_SessionJoinCheck
    {
        "SessionJoinCheck",
        "8B 8B F8 0E 06 00 8D 41 FA 83 F8 03 77 56"
    };

    // Functions: Simulation
    // players_set_machines(mask, identifiers): replaces the machines of the engine (A9 sec. 13.2).
    inline constexpr Signature k_SetMachines
    {
        "SetMachines",
        "48 8B C4 48 89 58 08 48 89 70 10 48 89 78 18 55 41 54 41 55 41 56 41 57 48 8D 68 C8 48 81 EC 10 01 00 00 48 8B 05 ?? ?? ?? ?? 48 33 C4 48 89 45 00 44 8B 05 ?? ?? ?? ?? 48 8B F2 65 48 8B 04 25 58 00 00 00"
    };

    // simulation_player_update_apply_added(update): creates the player of an "added" player update (A9 sec. 13.2).
    inline constexpr Signature k_ApplyPlayerAdded
    {
        "ApplyPlayerAdded",
        "48 8B C4 48 89 58 10 48 89 70 18 48 89 78 20 55 41 54 41 55 41 56 41 57 48 8D 68 A1 48 81 EC 00 01 00 00 48 8B 05 ?? ?? ?? ?? 48 33 C4 48 89 45 27 4C 63 39 33 DB 65 48 8B 04 25 58 00 00 00"
    };

    // simulation_players_apply_update(update) (0x418ff8): applies a player update by its type (+0xC: 0 left, 1 swap, 2 remove, 3 added,
    // 4 configuration). It has no stack cookie: the wildcards are the jump targets of the type switch and the TLS index (A9 sec. 13.15).
    // 1 hit in haloreach.dll build 1.3528.
    inline constexpr Signature k_ApplyPlayerUpdate
    {
        "ApplyPlayerUpdate",
        "48 83 EC 28 8B 51 0C 45 33 C0 4C 8B C9 85 D2 0F 84 ?? ?? ?? ?? 83 EA 01 0F 84 ?? ?? ?? ?? 83 EA 01 0F 84 ?? ?? ?? ?? 83 EA 01 0F 84 ?? ?? ?? ?? 83 FA 01 0F 85 ?? ?? ?? ?? 8B 15 ?? ?? ?? ?? 4C 63 11 65 48 8B 04 25 58 00 00 00"
    };

    // player_set_configuration(player, configuration): applies a "configuration" player update (name, appearance, team. A team change
    // kills the unit and starts the respawn timer) (A9 sec. 13.11).
    inline constexpr Signature k_ApplyPlayerConfiguration
    {
        "ApplyPlayerConfiguration",
        "48 89 5C 24 18 55 56 57 41 54 41 55 41 56 41 57 48 8D AC 24 30 FF FF FF 48 81 EC D0 01 00 00 48 8B 05 ?? ?? ?? ?? 48 33 C4 48 89 85 C0 00 00 00 65 48 8B 04 25 58 00 00 00 48 8B DA 44 8B E1 8B 0D ?? ?? ?? ??"
    };

    // First step of the interaction of a player (0x4ef34): the objects next to its unit (weapons, vehicles, equipment, objectives).
    // void(player, interaction). The engine runs both steps every frame for each local player only, through 0x4f280 (A9 M5d).
    inline constexpr Signature k_GatherNearbyInteractions
    {
        "GatherNearbyInteractions",
        "48 8B C4 48 89 58 18 55 56 57 41 54 41 55 41 56 41 57 48 8D A8 38 FC FF FF 48 81 EC 90 04 00 00 0F 29 70 B8 48 8B 05 ?? ?? ?? ?? 48 33 C4 48 89 85 70 03 00 00 44 8B 05 ?? ?? ?? ?? 4C 8B FA 65 48 8B 04 25 58 00 00 00 8B F9"
    };

    // Next weapon slot of a unit (0x48b618): int16(unit, from slot, direction). What a local client asks for when its player
    // switches weapon (A17 sec. 6, D120).
    inline constexpr Signature k_NextWeaponSlot
    {
        "NextWeaponSlot",
        "40 53 48 83 EC 20 44 8B 0D ?? ?? ?? ?? 65 48 8B 04 25 58 00 00 00 41 BB 10 00 00 00 4A 8B 04 C8 44 0F B7 C9 4E 8B 1C 18 4F 8D 14 49 49 8B 43 50 4E 8B 4C D0 10 49 0F BF 99 8E 01 00 00"
    };

    // Next grenade type of a unit with grenades left (0x48b58c): int16(unit, from type, cycle) (A17 sec. 6, D120).
    inline constexpr Signature k_NextGrenadeType
    {
        "NextGrenadeType",
        "48 89 5C 24 08 44 8B 0D ?? ?? ?? ?? 33 DB 65 48 8B 04 25 58 00 00 00 45 0F B7 D8 41 BA 10 00 00 00 41 83 C8 FF 66 44 3B C2 0F B7 C9"
    };

    // Next zoom level of the weapon of a unit (0x485838): int16(unit, current level), -1 at the end of the cycle (A17 sec. 6, D120).
    inline constexpr Signature k_NextZoomLevel
    {
        "NextZoomLevel",
        "48 8B C4 48 89 58 08 48 89 68 10 48 89 70 18 48 89 78 20 41 54 41 56 41 57 48 83 EC 20 41 BF 01 00 00 00 0F B7 FA 41 8A F7 48 83 CB FF E8 ?? ?? ?? ?? 8B 0D ?? ?? ?? ?? 44 8B C0"
    };

    // Whether a unit can zoom now (0x4852a0): bool(unit) (A17 sec. 6, D120).
    inline constexpr Signature k_CanUnitZoom
    {
        "CanUnitZoom",
        "48 8B C4 48 89 58 08 48 89 68 10 48 89 70 18 48 89 78 20 41 54 41 56 41 57 48 83 EC 20 8B 15 ?? ?? ?? ?? 41 83 CF FF 65 48 8B 04 25 58 00 00 00 40 B7 01 41 BC 10 00 00 00 8B D9"
    };

    // Functions: unit facts of the client (VW.9c.2). Each is 1 hit in haloreach.dll build 1.3528, MD5 012f4079af9a8bb0f624c178af8e0753.
    // WeaponZoomMagnification (0x4856b8): float(unit, zoom level int16): the magnification of the weapon in the hands at that level, 1.0 when it has none (A17 sec. 9.1, VW.9c.2).
    inline constexpr Signature k_WeaponZoomMagnification
    {
        "WeaponZoomMagnification",
        "48 89 5C 24 10 48 89 6C 24 18 56 57 41 54 41 56 41 57 48 83 EC 30 0F 29 74 24 20 0F B7 DA F3 0F"
    };

    // WeaponMovePenalty (0x4bc51c): float(weapon, is forward byte, is zoomed byte): the fraction the weapon takes off a move axis (forward = 1, side = 0) (VW.9c.2).
    inline constexpr Signature k_WeaponMovePenalty
    {
        "WeaponMovePenalty",
        "40 53 48 83 EC 20 44 8B 0D ?? ?? ?? ?? 41 8A D8 65 48 8B 04 25 58 00 00 00 41 BA 10 00 00 00 44"
    };

    // UnitCrouchGate (0x4ac810): bool(unit): the unit object kind says crouch is gated (the toggle does not apply). The kind table gives the check, it is a dispatch (VW.9c.2).
    inline constexpr Signature k_UnitCrouchGate
    {
        "UnitCrouchGate",
        "48 83 EC 28 8B 15 ?? ?? ?? ?? 65 48 8B 04 25 58 00 00 00 41 B9 10 00 00 00 44 8B D1 48 8B 04 D0 0F B7 D1 4E 8B 0C 08 4C 8D 04 52 49 8B 41 50 4A 8B 54 C0 10 48 0F BF 82 B2 01 00 00 4C 63 04 10 33 D2 41 83 F8 FF 74 0D 48 8D 0D ?? ?? ?? ?? 4A 8B 0C C1 EB 03 48 8B CA 48 8B 41 38"
    };

    // UnitForcedCrouch (0x4abee8): bool(unit): the unit is forced to crouch (VW.9c.2).
    inline constexpr Signature k_UnitForcedCrouch
    {
        "UnitForcedCrouch",
        "48 8B C4 48 89 58 08 48 89 68 10 48 89 70 18 48 89 78 20 41 54 41 56 41 57 48 83 EC 20 8B 15 ?? ?? ?? ?? 4C 8D 25 ?? ?? ?? ?? 65 48 8B 04 25 58 00 00 00 33"
    };

    // UnitCrouchToggleBlocked (0x48be48): bool(unit): the crouch toggle is cleared for this unit (VW.9c.2).
    inline constexpr Signature k_UnitCrouchToggleBlocked
    {
        "UnitCrouchToggleBlocked",
        "48 89 5C 24 08 48 89 74 24 10 48 89 7C 24 18 41 56 48 83 EC 20 8B 15 ?? ?? ?? ?? 65 48 8B 04 25 58 00 00 00 44"
    };

    // WeaponNextZoomLevel (0x4bc624): int16(weapon, level): the next zoom level of a weapon, -1 at the end of the cycle or when it has no levels (VW.9c.2).
    inline constexpr Signature k_WeaponNextZoomLevel
    {
        "WeaponNextZoomLevel",
        "48 89 5C 24 08 48 89 74 24 10 57 48 83 EC 20 44 8B 05 ?? ?? ?? ?? 48 8D 35 ?? ?? ?? ?? 65 48 8B 04 25 58 00 00 00 0F"
    };

    // UnitFrameOfReference (0x5dddc): int32(unit, out transform): the handle of the object whose frame the unit looks from, -1 if none. It writes the transform (VW.9c.2).
    inline constexpr Signature k_UnitFrameOfReference
    {
        "UnitFrameOfReference",
        "48 89 5C 24 08 48 89 74 24 10 48 89 7C 24 18 41 56 48 83 EC 20 48 8B FA 44 8B F1 E8 ?? ?? ?? ??"
    };

    // ControlsLocked (0x61830): bool(input user, local index, 0, 0): the controls of the user are locked (VW.9c.2).
    inline constexpr Signature k_ControlsLocked
    {
        "ControlsLocked",
        "48 89 5C 24 08 48 89 74 24 10 57 48 83 EC 20 44 8B 05 ?? ?? ?? ?? 8B FA 65 48 8B 04 25 58 00 00"
    };

    // PlayerLookOffset (0xda824): bool(player, dt float, out 2 floats, out 2 floats): the player is pushed off its aim (flinch, shake). The outputs are only written (VW.9c.2).
    inline constexpr Signature k_PlayerLookOffset
    {
        "PlayerLookOffset",
        "48 8B C4 48 89 58 08 4C 89 48 20 55 56 57 41 54 41 55 41 56 41 57 48 81 EC C0 00 00 00 0F 29 70"
    };

    // PlayerAltControl (0x715ac): bool(player, uint32* out, 0): the alternate control mode of the player, where the zoom comes from event 0x28 (VW.9c.2).
    inline constexpr Signature k_PlayerAltControl
    {
        "PlayerAltControl",
        "48 89 5C 24 08 48 89 74 24 10 57 48 83 EC 20 45 32 C9 44 8B D1 48 8B DA 83 F9 FF 0F 84 ?? ?? ??"
    };

    // TagFunctionEvaluate (0x174ed8): float(tag function data*, input float, previous float): the generic tag function evaluator. The
    // client calls it with the trigger curve of the unit tag (+0x498), the raw boost value and 0 (VW.9c.2 C).
    inline constexpr Signature k_TagFunctionEvaluate
    {
        "TagFunctionEvaluate",
        "48 8B C4 48 89 58 08 48 89 68 10 48 89 70 18 48 89 78 20 41 57 48 81 EC 90 00 00 00 8B 59 0C 4C 8D 3D ?? ?? ?? ?? 0F 29 70 E8 8B EB 0F 29 78 D8 0F 57 F6"
    };

    // TagFunctionScale (0x175524): float(tag function data*, value float): maps the evaluated value to the range of the function (VW.9c.2 C).
    inline constexpr Signature k_TagFunctionScale
    {
        "TagFunctionScale",
        "8B 51 0C 48 8D 0D ?? ?? ?? ?? 8B C2 48 C1 E8 1C 48 8B 0C C1 80 7C 91 02 00 75 43 F6 44 91 01 04 F3 0F 10 54 91 08"
    };

    // TagFlagTest (0x4c6b34): bool(tag handle uint16, flag int32): a flag word of the tag. Its first bytes load the two tag globals (the node to data index map and the segment base pointers), so the engine tag memory is read through them (VW.9c.2).
    inline constexpr Signature k_TagFlagTest
    {
        "TagFlagTest",
        "48 8B 05 ?? ?? ?? ?? 0F B7 C9 44 8B 44 C8 04 48 63 CA 49 8D 04 48 49 C1 E8 1C 48 03 C8 48 8D 05"
    };

    // Second step of the interaction of a player (0x4eb54): objects inside a cone of its weapon. Same arguments (A9 M5d).
    inline constexpr Signature k_GatherAimedInteractions
    {
        "GatherAimedInteractions",
        "48 89 5C 24 18 55 56 57 41 54 41 55 41 56 41 57 B8 C0 22 00 00 E8 ?? ?? ?? ?? 48 2B E0 0F 29 B4 24 B0 22 00 00 0F 29 BC 24 A0 22 00 00 44 0F 29 84 24 90 22 00 00 44 0F 29 8C 24 80 22 00 00 48 8B 05 ?? ?? ?? ?? 48 33 C4 48 89 84 24 70 22 00 00 44 8B 05 ?? ?? ?? ?? 45 33 E4 65 48 8B 04 25 58 00 00 00 4C 8B FA"
    };

    // collision_test_vector (0x12969c, A20): the ray test the bullets use. It is found by its RVA and checked twice: the unwind table must
    // say a function begins exactly there, and the bytes there must be its whole prologue (the pushes, then "mov eax, 0x16910 / call
    // alloca_probe / sub rsp, rax"). 0x16910 + the pushes = the 0x16948 of A20 sec. 2. Read from haloreach.dll MD5 012f4079...
    inline constexpr Signature k_CollisionTestVectorPrologue
    {
        "CollisionTestVectorPrologue",
        "48 89 5C 24 10 55 56 57 41 54 41 55 41 56 41 57 48 8D AC 24 F0 97 FE FF B8 10 69 01 00 E8 ?? ?? ?? ?? 48 2B E0"
    };

    // Functions: Map variant
    // Initializes a map variant buffer to the empty one (0x6bff8): void*(buffer). With the id -1 it does not look at any tag.
    // 1 hit in haloreach.dll build 1.3528 (Q106).
    inline constexpr Signature k_MapVariantInit
    {
        "MapVariantInit",
        "48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 57 48 83 EC 20 33 ED 48 8D B9 FC 14 00 00 33 C0 48 89 A9 B8 02 00 00"
    };

    // Decodes the map variant chunk of a .mvar file (0x17adac): checks the SHA1 of the chunk and reads its bitstream into a buffer the
    // initializer prepared. bool(chunk header, buffer). 1 hit in haloreach.dll build 1.3528 (Q106).
    inline constexpr Signature k_MapVariantDecode
    {
        "MapVariantDecode",
        "48 89 5C 24 18 48 89 74 24 20 55 57 41 56 48 8D AC 24 70 FF FF FF 48 81 EC 90 01 00 00 48 8B 05 ?? ?? ?? ?? 48 33 C4 48 89 85 80 00 00 00 8B 41 20 4C 8B F2 48 8B F9 33 DB 0F C8"
    };

    inline constexpr std::uint32_t k_CollisionTestVectorRva{ 0x12969C };

    // The game's HUD (chud) passes inside the render of a view (A21 sec. 5, D325). Each one is void(uint32 user). They are found by their RVA and
    // checked like collision_test_vector: the unwind table must say a function begins there and the bytes must be the start of its prologue.
    // Read from haloreach.dll MD5 012f4079...
    // The draw of the list of widgets (0x2c29f8).
    inline constexpr Signature k_ChudDrawPrologue
    {
        "ChudDrawPrologue",
        "48 89 5C 24 10 48 89 6C 24 18 48 89 74 24 20 57 41 54 41 55 41 56 41 57 48 81 EC 70 03 00 00"
    };

    // The second list of widgets, drawn after the screen effects (0x2c2628).
    inline constexpr Signature k_ChudDrawLatePrologue
    {
        "ChudDrawLatePrologue",
        "48 8B C4 48 89 58 08 48 89 68 10 48 89 70 18 48 89 78 20 41 54 41 56"
    };

    // The 3D part of the HUD (0x2c279c).
    inline constexpr Signature k_ChudDraw3DPrologue
    {
        "ChudDraw3DPrologue",
        "48 8B C4 48 89 58 08 48 89 68 10 48 89 70 18 48 89 78 20 41 56 48 83 EC 30"
    };

    inline constexpr std::uint32_t k_ChudDrawRva{ 0x2C29F8 };
    inline constexpr std::uint32_t k_ChudDrawLateRva{ 0x2C2628 };
    inline constexpr std::uint32_t k_ChudDraw3DRva{ 0x2C279C };

    // s_render_projection (A21 sec. 3): a global of haloreach.dll, not TLS. The game rebuilds it for each view before it draws it, so the
    // Present of the frame finds the projection of the last view. 0x30 floats, read only.
    inline constexpr std::uint32_t k_RenderProjectionRva{ 0xC9FB70 };

    // DAT_180c4d958 (VW.5c, RE not verified live): the pointer to the buffer of node matrices the game interpolates the frame with. Null when
    // the game does not interpolate. A global of haloreach.dll, read only.
    inline constexpr std::uint32_t k_InterpolationBufferRva{ 0xC4D958 };

    // The function that builds the camera result of a user from the WORK of the observer (A21 sec. 6, VW.5b): void(uint32 user, void* out).
    // Found by RVA like the HUD passes. Read from haloreach.dll MD5 012f4079...
    inline constexpr Signature k_ObserverCameraPrologue
    {
        "ObserverCameraPrologue",
        "48 8B C4 48 89 58 08 48 89 70 18 48 89 78 20 48 89 50 10 55 41 54 41 55 41 56"
    };

    inline constexpr std::uint32_t k_ObserverCameraRva{ 0xC9CF8 };

    // The function that makes an output user (view) show a player (A21 sec. 9, VW.9a, D340): void(uint32 outputUser, uint32 playerHandle). The
    // theater uses it to change who a view watches. It is called, not hooked. Found by RVA like the HUD passes.
    // Read from haloreach.dll MD5 012f4079...
    inline constexpr Signature k_AssignOutputUserPrologue
    {
        "AssignOutputUserPrologue",
        "48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 57 41 54 41 55 41 56 41 57"
    };

    inline constexpr std::uint32_t k_AssignOutputUserRva{ 0x53D7C };

    // The first person model pass (A21 sec. 8, D331): void(void* camera, uint32 user, uint8 pass). It draws the arms and the weapon of the
    // local player in first person. Hooked to skip it (option Hide first person, or while Drive moves the camera).
    // Read from haloreach.dll MD5 012f4079...
    inline constexpr Signature k_FirstPersonModelPrologue
    {
        "FirstPersonModelPrologue",
        "48 89 5C 24 08 57 48 83 EC 20 4C 63 CA 45 8A D0"
    };

    // The test the game asks to know if an object is the first person unit of a user (A21 sec. 8, D331): bool(uint32 user, uint32 objectHandle),
    // returned in AL. The render of the biped and some effects call it. Hooked to answer false while Drive moves the camera, so the game draws
    // the player's own biped and the third person effects.
    inline constexpr Signature k_IsFirstPersonUnitPrologue
    {
        "IsFirstPersonUnitPrologue",
        "48 83 EC 28 44 8B CA 45 33 D2 48 63 D1 41 83 F9 FF"
    };

    // The pass between the entry and the exit of the first person draw (A21 sec. 8, D331): void(void* view, int32 pass). It queues the model,
    // sets the first person camera, draws the list, restores the normal camera and pops (it is called with 0xB by 0x26C6DC and with 0
    // twice by 0x26D740). Measured in VW.6.y.2 A5: the call with 0xB is the one that writes the depth of the arms and weapon, so the
    // overlay mask (VW.6.y.2 B) is made around it.
    inline constexpr Signature k_FirstPersonDepthPassPrologue
    {
        "FirstPersonDepthPassPrologue",
        "48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 57 48 83 EC 20"
    };

    // The opaque color pass of the view (A21 sec. 8 CORRECTION, D334): void(void* view), called once per view by 0x26C6DC after the first person
    // lists of the view and before the effects and the post. It draws the color of the opaque world (0x2989e4 with list 0), so the overlay
    // is drawn at its exit, over the world. Read from haloreach.dll MD5 012f4079... on 2026-11-19.
    inline constexpr Signature k_OpaqueColorPassPrologue
    {
        "OpaqueColorPassPrologue",
        "48 8B C4 48 89 58 08 48 89 78 10 55 48 8D 68 A1 48 81 EC 90 00 00 00"
    };

    inline constexpr std::uint32_t k_FirstPersonModelRva{ 0x286EBC };
    inline constexpr std::uint32_t k_FirstPersonDepthPassRva{ 0x26DA08 };
    inline constexpr std::uint32_t k_OpaqueColorPassRva{ 0x26E500 };
    inline constexpr std::uint32_t k_IsFirstPersonUnitRva{ 0x266104 };

    // The prologue must begin exactly at the RVA (no slack: a moved function is not called).
    inline constexpr std::uint32_t k_CollisionTestVectorWindow{ 0 };

    // Same rule for the three HUD passes.
    inline constexpr std::uint32_t k_ChudWindow{ 0 };

    // Same rule for the observer camera.
    inline constexpr std::uint32_t k_ObserverCameraWindow{ 0 };

    // Same rule for the first person functions.
    inline constexpr std::uint32_t k_FirstPersonWindow{ 0 };

    // Same rule for the view assignment.
    inline constexpr std::uint32_t k_AssignOutputUserWindow{ 0 };
}