module Platform.Host.Hook;
import :SessionJoinCheck;

import Platform.Memory.Type;
import Platform.Hook.System;
import std;

namespace
{
    namespace Signature = Platform::Memory::Type::Signature;

    // The "ja" of the SessionJoinCheck signature starts at +12.
    constexpr std::size_t k_JumpOffset{ 12 };

    constexpr std::uint8_t k_JumpIfAbove{ 0x77 };
    constexpr std::array<std::uint8_t, 1> k_JumpAlways{ 0xEB };
}

namespace Platform::Host::Hook
{
    auto SessionJoinCheckPatch::Install() -> bool
    {
        if (m_IsPatched) return true;
        if (m_LocalUserStore.GetRequestedCount() <= 1) return true;

        const std::uintptr_t match{ m_AOBService.FindPattern(Signature::SessionJoinCheck) };
        if (match == 0) return false;

        auto* jump{ reinterpret_cast<std::uint8_t*>(match + k_JumpOffset) };

        if (*jump != k_JumpIfAbove)
        {
            m_LogsService.Message("[SessionJoinCheckPatch] ERROR: Unexpected byte {:02X} at the jump.", *jump);
            return false;
        }

        if (!Platform::Hook::System::WritePatch(jump, k_JumpAlways, m_OriginalBytes,
            "[SessionJoinCheckPatch]", m_LogsService))
        {
            return false;
        }

        m_PatchAddress = jump;
        m_IsPatched = true;
        return true;
    }

    auto SessionJoinCheckPatch::Uninstall() -> void
    {
        if (!m_IsPatched) return;

        Platform::Hook::System::RestorePatch(m_PatchAddress, m_OriginalBytes,
            "[SessionJoinCheckPatch]", m_LogsService);

        m_PatchAddress = nullptr;
        m_OriginalBytes.clear();
        m_IsPatched = false;
    }
}