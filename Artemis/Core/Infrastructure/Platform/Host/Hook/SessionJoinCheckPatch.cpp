module Platform.Host.Hook;
import :SessionJoinCheck;

import Platform.Host.Type;
import Platform.Memory.Type;
import Platform.Hook.System;
import std;

namespace
{
    namespace Signature = Platform::Memory::Type::Signature;

    using Platform::Host::Type::k_JumpIfAboveOpcode;
    using Platform::Host::Type::k_JumpAlwaysPatch;

    // The "ja" of the SessionJoinCheck signature starts at +12.
    constexpr std::size_t k_JumpOffset{ 12 };
}

namespace Platform::Host::Hook
{
    auto SessionJoinCheckPatch::Install() -> bool
    {
        if (m_IsPatched) return true;
        if (m_LocalUserStore.GetRequestedCount() <= 1) return true;

        const std::uintptr_t match{ m_AOBService.FindPattern(Signature::k_SessionJoinCheck) };
        if (match == 0) return false;

        auto* jump{ reinterpret_cast<std::uint8_t*>(match + k_JumpOffset) };

        if (*jump != k_JumpIfAboveOpcode)
        {
            m_LogsService.Message("[SessionJoinCheckPatch] ERROR: Unexpected byte {:02X} at the jump.", *jump);
            return false;
        }

        if (!Platform::Hook::System::WritePatch(jump, k_JumpAlwaysPatch, m_OriginalBytes,
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