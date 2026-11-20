module;

#include <windows.h>
#include <intrin.h>

module Engine.Base.Pov.Hook;
import :Probe;

import Engine.Base.Hook.System;
import Engine.Base.Memory.Type;
import std;

namespace
{
    using Engine::Base::Hook::System::k_DrainTimeout;
    using Engine::Base::Memory::Type::Signature::Signature;
    using Engine::Base::Memory::Type::Signature::k_AssignOutputUserPrologue;
    using Engine::Base::Memory::Type::Signature::k_AssignOutputUserRva;
    using Engine::Base::Memory::Type::Signature::k_CursorUpdatePrologue;
    using Engine::Base::Memory::Type::Signature::k_CursorUpdateRva;
    using Engine::Base::Memory::Type::Signature::k_PlayerAssignUnitPrologue;
    using Engine::Base::Memory::Type::Signature::k_PlayerAssignUnitRva;
    using Engine::Base::Memory::Type::Signature::k_PlayerUnitPrologue;
    using Engine::Base::Memory::Type::Signature::k_PlayerUnitRva;
    using Engine::Base::Memory::Type::Signature::k_PovProbeWindow;
    using Engine::Base::Memory::Type::Signature::k_ViewUnitPrologue;
    using Engine::Base::Memory::Type::Signature::k_ViewUnitRva;
    using Engine::Base::Pov::Type::PovProbeKind;

    using Engine::Base::Pov::Type::k_PovProbeHookCount;
    using Engine::Base::Pov::Type::k_PovProbeUnreadable;

    // One function to hook: its signature, the detour and where the pointer to the original goes.
    struct ProbeTarget
    {
        const Signature* Prologue{};
        std::uint32_t Rva{};
        const char* Name{};
        void* Hook{};
        void** Original{};
    };

    // return: The u32 the pointer points to, or k_PovProbeUnreadable if it cannot be read. The pointer is an argument of the hooked function.
    auto ReadFirstValue(std::uint64_t pointer) -> std::uint64_t
    {
        __try
        {
            return *reinterpret_cast<const std::uint32_t*>(pointer);
        }
        __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
        {
            return k_PovProbeUnreadable;
        }
    }
}

namespace Engine::Base::Pov::Hook
{
    // note: The five hooked functions below follow one rule: the in flight scope spans the call of the original, the original is always called
    // when there is one (the statics are cleared only after the hook is removed and drained), and nothing here allocates, logs or waits. The
    // window is the one of the other detours of the project (ObserverCameraDetour): a thread that read the original pointer after Uninstall
    // cleared it returns without calling it, and a thread between the jump into the detour and its in flight scope is not counted by the drain
    // (a few instructions). Uninstall disables the hooks first, so only threads already past the jump can meet either case.
    //
    // All five forward eight integer arguments and the return value, so the original receives exactly what the game passed: a function with up
    // to eight integer arguments is called as the game called it. The slots past the real arguments read the stack of the caller, which is
    // always mapped. The three functions of known signature note the low 32 bits of the first two arguments.
    auto __fastcall PovProbeDetours::HookedViewPlayer(std::uint64_t a0, std::uint64_t a1, std::uint64_t a2, std::uint64_t a3, std::uint64_t a4,
        std::uint64_t a5, std::uint64_t a6, std::uint64_t a7) -> std::uint64_t
    {
        Engine::Base::Hook::System::InFlightScope scope{ s_InFlight };

        const PovProbeWideFunction original{ s_ViewPlayerOriginal };
        if (original == nullptr) return 0;

        PovProbeDetours* const self{ s_Instance.load() };
        if (self != nullptr && self->m_Store.IsActive())
        {
            self->m_Store.Record(PovProbeKind::ViewPlayer, ::GetCurrentThreadId(), reinterpret_cast<std::uint64_t>(_ReturnAddress()),
                { static_cast<std::uint32_t>(a0), static_cast<std::uint32_t>(a1), 0, 0 });
        }

        return original(a0, a1, a2, a3, a4, a5, a6, a7);
    }

    auto __fastcall PovProbeDetours::HookedViewUnit(std::uint64_t a0, std::uint64_t a1, std::uint64_t a2, std::uint64_t a3, std::uint64_t a4,
        std::uint64_t a5, std::uint64_t a6, std::uint64_t a7) -> std::uint64_t
    {
        Engine::Base::Hook::System::InFlightScope scope{ s_InFlight };

        const PovProbeWideFunction original{ s_ViewUnitOriginal };
        if (original == nullptr) return 0;

        PovProbeDetours* const self{ s_Instance.load() };
        if (self != nullptr && self->m_Store.IsActive())
        {
            self->m_Store.Record(PovProbeKind::ViewUnit, ::GetCurrentThreadId(), reinterpret_cast<std::uint64_t>(_ReturnAddress()),
                { static_cast<std::uint32_t>(a0), static_cast<std::uint32_t>(a1), 0, 0 });
        }

        return original(a0, a1, a2, a3, a4, a5, a6, a7);
    }

    auto __fastcall PovProbeDetours::HookedPlayerUnit(std::uint64_t a0, std::uint64_t a1, std::uint64_t a2, std::uint64_t a3, std::uint64_t a4,
        std::uint64_t a5, std::uint64_t a6, std::uint64_t a7) -> std::uint64_t
    {
        Engine::Base::Hook::System::InFlightScope scope{ s_InFlight };

        const PovProbeWideFunction original{ s_PlayerUnitOriginal };
        if (original == nullptr) return 0;

        PovProbeDetours* const self{ s_Instance.load() };
        if (self != nullptr && self->m_Store.IsActive())
        {
            self->m_Store.Record(PovProbeKind::PlayerUnit, ::GetCurrentThreadId(), reinterpret_cast<std::uint64_t>(_ReturnAddress()),
                { static_cast<std::uint32_t>(a0), static_cast<std::uint32_t>(a1), 0, 0 });
        }

        return original(a0, a1, a2, a3, a4, a5, a6, a7);
    }

    // note: The signature of the function is not confirmed: rcx, rdx, r8 and r9 are noted without interpreting them.
    auto __fastcall PovProbeDetours::HookedPlayerAssignUnit(std::uint64_t a0, std::uint64_t a1, std::uint64_t a2, std::uint64_t a3, std::uint64_t a4,
        std::uint64_t a5, std::uint64_t a6, std::uint64_t a7) -> std::uint64_t
    {
        Engine::Base::Hook::System::InFlightScope scope{ s_InFlight };

        const PovProbeWideFunction original{ s_PlayerAssignUnitOriginal };
        if (original == nullptr) return 0;

        PovProbeDetours* const self{ s_Instance.load() };
        if (self != nullptr && self->m_Store.IsActive())
        {
            self->m_Store.Record(PovProbeKind::PlayerAssignUnit, ::GetCurrentThreadId(), reinterpret_cast<std::uint64_t>(_ReturnAddress()), { a0, a1, a2, a3 });
        }

        return original(a0, a1, a2, a3, a4, a5, a6, a7);
    }

    // note: Same forwarding as PlayerAssignUnit. The first value is read before the call (the function may change it) and guarded.
    auto __fastcall PovProbeDetours::HookedCursor(std::uint64_t thisPointer, std::uint64_t valuePointer, std::uint64_t a2, std::uint64_t a3,
        std::uint64_t a4, std::uint64_t a5, std::uint64_t a6, std::uint64_t a7) -> std::uint64_t
    {
        Engine::Base::Hook::System::InFlightScope scope{ s_InFlight };

        const PovProbeWideFunction original{ s_CursorOriginal };
        if (original == nullptr) return 0;

        PovProbeDetours* const self{ s_Instance.load() };
        if (self != nullptr && self->m_Store.IsActive())
        {
            self->m_Store.RecordCursor(thisPointer, ReadFirstValue(valuePointer));
        }

        return original(thisPointer, valuePointer, a2, a3, a4, a5, a6, a7);
    }

    auto PovProbeDetours::Install() -> bool
    {
        const std::lock_guard<std::mutex> lock{ m_Mutex };

        if (m_IsInstalled.load()) return true;

        const std::array<ProbeTarget, k_PovProbeHookCount> targets
        { {
            { &k_AssignOutputUserPrologue, k_AssignOutputUserRva, "53D7C", reinterpret_cast<void*>(&HookedViewPlayer),
                reinterpret_cast<void**>(&s_ViewPlayerOriginal) },
            { &k_ViewUnitPrologue, k_ViewUnitRva, "54070", reinterpret_cast<void*>(&HookedViewUnit),
                reinterpret_cast<void**>(&s_ViewUnitOriginal) },
            { &k_PlayerUnitPrologue, k_PlayerUnitRva, "53CA4", reinterpret_cast<void*>(&HookedPlayerUnit),
                reinterpret_cast<void**>(&s_PlayerUnitOriginal) },
            { &k_PlayerAssignUnitPrologue, k_PlayerAssignUnitRva, "45814", reinterpret_cast<void*>(&HookedPlayerAssignUnit),
                reinterpret_cast<void**>(&s_PlayerAssignUnitOriginal) },
            { &k_CursorUpdatePrologue, k_CursorUpdateRva, "2D2018", reinterpret_cast<void*>(&HookedCursor),
                reinterpret_cast<void**>(&s_CursorOriginal) },
        } };

        s_Instance.store(this);

        std::uint32_t installed{ 0 };
        for (std::size_t index{ 0 }; index < targets.size(); ++index)
        {
            const ProbeTarget& target{ targets[index] };

            // The window is 0: the prologue must begin exactly at the RVA.
            void* const functionAddress{ reinterpret_cast<void*>(m_AOBService.FindFunctionAtRva(*target.Prologue, target.Rva, k_PovProbeWindow)) };
            if (functionAddress == nullptr)
            {
                m_Log.Message("[PovProbeDetours] ERROR: The function {} was not found at rva {:X}, its hook is not installed.", target.Name, target.Rva);
                continue;
            }

            if (!Engine::Base::Hook::System::InstallDetour(functionAddress, target.Hook, target.Original, "[PovProbeDetours]", m_Log))
            {
                *target.Original = nullptr;
                continue;
            }

            m_Addresses[index] = functionAddress;
            ++installed;
            m_Log.Message("[PovProbeDetours] INFO: The function {} is hooked at rva {:X}.", target.Name, target.Rva);
        }

        if (installed == 0)
        {
            s_Instance.store(nullptr);
            return false;
        }

        m_IsInstalled.store(true);
        return true;
    }

    auto PovProbeDetours::Uninstall() -> void
    {
        const std::lock_guard<std::mutex> lock{ m_Mutex };

        if (!m_IsInstalled.load()) return;

        for (void* const address : m_Addresses)
        {
            if (address != nullptr) Engine::Base::Hook::System::DisableDetour(address);
        }

        if (!Engine::Base::Hook::System::WaitForDrain(s_InFlight, k_DrainTimeout))
        {
            m_Log.Message("[PovProbeDetours] WARNING: A call was still running when the hooks were removed.");
        }

        for (void*& address : m_Addresses)
        {
            if (address != nullptr) Engine::Base::Hook::System::RemoveDetour(address, "[PovProbeDetours]", m_Log);
            address = nullptr;
        }

        // Only now: until the hooks are removed a thread can still be inside a detour.
        s_ViewPlayerOriginal = nullptr;
        s_ViewUnitOriginal = nullptr;
        s_PlayerUnitOriginal = nullptr;
        s_PlayerAssignUnitOriginal = nullptr;
        s_CursorOriginal = nullptr;
        s_Instance.store(nullptr);
        m_IsInstalled.store(false);
    }
}