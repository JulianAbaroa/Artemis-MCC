module Engine.Base.Host.System;

import Engine.Base.Memory.Type;
import Engine.Base.Host.Type;
import std;

namespace
{
    namespace Signature = Engine::Base::Memory::Type::Signature;

    using Engine::Base::Host::Type::k_HostModuleName;

    // Inside the GameManager signature, "mov [rip + disp32], rax" stores the vtable in the object: disp32 at +10, next instruction at +14.
    constexpr std::size_t k_StoreDisplacementOffset{ 10 };
    constexpr std::size_t k_StoreNextInstructionOffset{ 14 };
}

namespace Engine::Base::Host::System
{
    auto HostService::GetFunctionAddress(std::size_t index) -> void*
    {
        void** vtable{ m_VTable.load(std::memory_order_acquire) };

        if (vtable == nullptr)
        {
            vtable = this->ResolveVTable();
            if (vtable == nullptr) return nullptr;

            m_VTable.store(vtable, std::memory_order_release);
        }

        return vtable[index];
    }

    auto HostService::ResolveVTable() -> void**
    {
        const std::uintptr_t match{ m_AOBService.FindPattern(Signature::k_GameManager, k_HostModuleName) };
        if (match == 0) return nullptr;

        std::int32_t displacement{ 0 };
        std::memcpy(&displacement, reinterpret_cast<const void*>(match + k_StoreDisplacementOffset), sizeof(displacement));

        const std::uintptr_t gameManager{ static_cast<std::uintptr_t>(
            static_cast<std::intptr_t>(match + k_StoreNextInstructionOffset) + displacement) };

        void** vtable{ *reinterpret_cast<void***>(gameManager) };

        if (vtable == nullptr)
        {
            m_Log.Message("[HostService] ERROR: The game manager has no vtable yet.");
            return nullptr;
        }

        m_Log.Message("[HostService] INFO: Game manager found at {:016X}, vtable at {:016X}.",
            gameManager, reinterpret_cast<std::uintptr_t>(vtable));

        return vtable;
    }
}