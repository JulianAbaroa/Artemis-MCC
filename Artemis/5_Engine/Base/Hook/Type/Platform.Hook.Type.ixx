export module Platform.Hook.Type;

import std;

export namespace Platform::Hook::Type
{
    // First byte of a rel32 jump, the one a detour writes over the start of the hooked function.
    inline constexpr std::uint8_t k_JumpRel32Opcode{ 0xE9 };

    // Offset of the thread local storage array in the thread environment block (gs:[0x58]).
    inline constexpr std::uintptr_t k_TlsArrayOffset{ 0x58 };

    // Sanity limit of the slot of the engine thread context. Anything above it is not a valid slot.
    inline constexpr std::uint32_t k_MaxTlsIndex{ 1000 };

    // The match of Signature::k_TelemetryIdModifier is a "mov ecx, [rip+rel32]": the rel32 is at +2 and the instruction is 6 bytes long.
    inline constexpr std::uintptr_t k_TlsIndexRelOffset{ 2 };
    inline constexpr std::uintptr_t k_TlsIndexNextOffset{ 6 };
}