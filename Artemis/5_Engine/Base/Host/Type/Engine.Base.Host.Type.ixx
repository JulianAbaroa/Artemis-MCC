export module Engine.Base.Host.Type;

import std;

export namespace Engine::Base::Host::Type
{
    // Module of MCC, the host that loads the game DLLs and gives them the game manager.
    inline constexpr const wchar_t* k_HostModuleName{ L"MCC-Win64-Shipping.exe" };

    // Module of the game MCC loads for Reach. It is loaded again for every match, so its base changes.
    inline constexpr const wchar_t* k_GameModuleName{ L"haloreach.dll" };

    // Entries of the game manager vtable (byte offset / 8). Verified on MCC 1.3528 (A9_SplitScreen.txt sec. 6).
    inline constexpr std::size_t k_LocalUserGetProfileIndex{ 34 };
    inline constexpr std::size_t k_InputUpdateIndex{ 36 };
    inline constexpr std::size_t k_LocalUserGetPlayerIndex{ 88 };
    inline constexpr std::size_t k_LocalUserGetGamepadMappingIndex{ 116 };

    // Bytes of the profile and of the gamepad mapping MCC keeps per user.
    inline constexpr std::size_t k_PlayerProfileSize{ 0xACC };
    inline constexpr std::size_t k_GamepadMappingSize{ 0x42 };

    // Profile of a local user: look settings (fov, sensitivity, dead zones, inverted look), loadouts and key bindings.
    // note: Opaque. Artemis copies it without reading the fields.
    struct PlayerProfile
    {
        std::array<std::byte, k_PlayerProfileSize> Data{};
    };

    // Gamepad button mapping of a local user.
    // note: Opaque. Artemis copies it without reading the fields.
    struct GamepadMapping
    {
        std::array<std::byte, k_GamepadMappingSize> Data{};
    };

    // Signature of the MCC function that reports a local user to the engine.
    // param nameSize: Capacity of the name buffer in bytes.
    // return: False if there is no user in that slot.
    using LocalUserGetPlayerFunction = auto(__fastcall*)(void* gameManager, std::uint64_t* xuid, wchar_t* name,
        std::uint32_t nameSize, std::int32_t player) -> bool;

    // Signature of the MCC function that returns the profile of a user.
    using LocalUserGetProfileFunction = auto(__fastcall*)(void* gameManager, std::uint64_t xuid) -> PlayerProfile*;

    // Signature of the MCC function that returns the gamepad mapping of a user.
    using LocalUserGetGamepadMappingFunction = auto(__fastcall*)(void* gameManager, std::uint64_t xuid) -> GamepadMapping*;

    // Signature of the engine function that decides if the network session accepts join requests.
    // return: 0 when it accepts them, an error code otherwise.
    using CanAcceptAnyJoinRequestFunction = auto(__fastcall*)(void* session) -> std::int32_t;

    // Jump of the SessionJoinCheck signature: the "ja" opcode of the engine, and the patch bytes ("jmp") that replace it.
    inline constexpr std::uint8_t k_JumpIfAboveOpcode{ 0x77 };
    inline constexpr std::array<std::uint8_t, 1> k_JumpAlwaysPatch{ 0xEB };
}