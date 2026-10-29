module Engine.Base.Host.Hook;
import :SessionJoinCheck;

import Engine.Base.Host.Type;
import Engine.Base.Memory.Type;
import Engine.Base.Hook.System;
import std;

namespace
{
    namespace Signature = Engine::Base::Memory::Type::Signature;

    using Engine::Base::Host::Type::k_JumpIfAboveOpcode;
    using Engine::Base::Host::Type::k_JumpAlwaysPatch;

    // The "ja" of the SessionJoinCheck signature starts at +12.
    constexpr std::size_t k_JumpOffset{ 12 };
}

namespace Engine::Base::Host::Hook
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
            m_Log.Message("[SessionJoinCheckPatch] ERROR: Unexpected byte {:02X} at the jump.", *jump);
            return false;
        }

        if (!Engine::Base::Hook::System::WritePatch(jump, k_JumpAlwaysPatch, m_OriginalBytes,
            "[SessionJoinCheckPatch]", m_Log))
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

        Engine::Base::Hook::System::RestorePatch(m_PatchAddress, m_OriginalBytes,
            "[SessionJoinCheckPatch]", m_Log);

        m_PatchAddress = nullptr;
        m_OriginalBytes.clear();
        m_IsPatched = false;
    }
}