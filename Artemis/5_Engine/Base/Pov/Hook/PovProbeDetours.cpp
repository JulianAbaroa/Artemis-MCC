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
    using Engine::Base::Memory::Type::Signature::k_ProbeScreenField154;
    using Engine::Base::Memory::Type::Signature::k_ProbeScreenField1AC;
    using Engine::Base::Memory::Type::Signature::k_ProbeScreenField1C0;
    using Engine::Base::Memory::Type::Signature::k_ProbeScreenField1C4;
    using Engine::Base::Memory::Type::Signature::k_ProbeScreenFlags;
    using Engine::Base::Memory::Type::Signature::k_ProbeScreenMask;
    using Engine::Base::Memory::Type::Signature::k_ProbeScreenState;
    using Engine::Base::Memory::Type::Signature::k_ProbeScreenWindow;
    using Engine::Base::Memory::Type::Signature::k_ScreenCreateAltRva;
    using Engine::Base::Memory::Type::Signature::k_ScreenCreateExPrologue;
    using Engine::Base::Memory::Type::Signature::k_ScreenCreateExRva;
    using Engine::Base::Memory::Type::Signature::k_ScreenCreatePrologue;
    using Engine::Base::Memory::Type::Signature::k_ScreenCreateRva;
    using Engine::Base::Memory::Type::Signature::k_PlayerAssignUnitRva;
    using Engine::Base::Memory::Type::Signature::k_PlayerUnitPrologue;
    using Engine::Base::Memory::Type::Signature::k_PlayerUnitRva;
    using Engine::Base::Memory::Type::Signature::k_PovProbeWindow;
    using Engine::Base::Memory::Type::Signature::k_ViewUnitPrologue;
    using Engine::Base::Memory::Type::Signature::k_ViewUnitRva;
    using Engine::Base::Pov::Type::PovProbeEvent;
    using Engine::Base::Pov::Type::PovProbeKind;
    using Engine::Base::Pov::Type::PovProbeScreen;
    using Engine::Base::Pov::State::PovProbeStore;
    using Engine::Base::Pov::Type::PovProbeScreenField;

    using Engine::Base::Pov::Type::k_PovProbeExtraWords;
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

    // return: Whether the fields of the screen at the pointer were read. A signed field is sign extended. The pointer is the this of the hooked update.
    auto ReadScreen(std::uint64_t pointer, PovProbeScreen& out) -> bool
    {
        __try
        {
            const auto* base{ reinterpret_cast<const std::uint8_t*>(pointer) };
            auto& values{ out.Values };
            values[static_cast<std::size_t>(PovProbeScreenField::Vtable)] = *reinterpret_cast<const std::uint64_t*>(base);
            values[static_cast<std::size_t>(PovProbeScreenField::Window)] = static_cast<std::uint64_t>(
                static_cast<std::int64_t>(*reinterpret_cast<const std::int32_t*>(base + k_ProbeScreenWindow)));
            values[static_cast<std::size_t>(PovProbeScreenField::Mask)] = *(base + k_ProbeScreenMask);
            values[static_cast<std::size_t>(PovProbeScreenField::Flags)] = *reinterpret_cast<const std::uint32_t*>(base + k_ProbeScreenFlags);
            values[static_cast<std::size_t>(PovProbeScreenField::State)] = *(base + k_ProbeScreenState);
            values[static_cast<std::size_t>(PovProbeScreenField::Field1AC)] = static_cast<std::uint64_t>(
                static_cast<std::int64_t>(*reinterpret_cast<const std::int32_t*>(base + k_ProbeScreenField1AC)));
            values[static_cast<std::size_t>(PovProbeScreenField::Field154)] = *reinterpret_cast<const std::uint32_t*>(base + k_ProbeScreenField154);
            values[static_cast<std::size_t>(PovProbeScreenField::Field1C0)] = *reinterpret_cast<const std::uint32_t*>(base + k_ProbeScreenField1C0);
            values[static_cast<std::size_t>(PovProbeScreenField::Field1C4)] = *reinterpret_cast<const std::uint32_t*>(base + k_ProbeScreenField1C4);
            return true;
        }
        __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
        {
            return false;
        }
    }

    // Notes a call of a function with up to four noted integer arguments.
    auto NoteCall(PovProbeStore& store, PovProbeKind kind, std::uint64_t caller, std::uint64_t a0, std::uint64_t a1, std::uint64_t a2,
        std::uint64_t a3) -> void
    {
        PovProbeEvent event{};
        event.Kind = kind;
        event.ThreadId = ::GetCurrentThreadId();
        event.Caller = caller;
        event.Args = { a0, a1, a2, a3 };
        store.Record(event);
    }

    // Notes a creation of a screen: the arguments after the context, the screen it returned and the stack from the caller of the creator.
    // note: The stack is the return addresses the system walks from here, starting at the one equal to caller (the frames of this helper and of the
    // detour before it do not count, so inlining does not change the result). Nothing is allocated and no engine memory is read.
    auto NoteCreate(PovProbeStore& store, PovProbeKind kind, std::uint64_t caller, std::uint64_t window, std::uint64_t name, std::uint64_t a4,
        std::uint64_t a5, std::uint64_t result) -> void
    {
        constexpr std::size_t k_CaptureFrames{ k_PovProbeExtraWords + 4 };
        std::array<void*, k_CaptureFrames> frames{};
        const std::size_t captured{ ::RtlCaptureStackBackTrace(0, static_cast<DWORD>(frames.size()), frames.data(), nullptr) };

        // The stack starts at the caller. If it is not among the frames the leading frames of our own DLL are skipped instead.
        std::size_t first{ captured };
        for (std::size_t index{ 0 }; index < captured; ++index)
        {
            if (reinterpret_cast<std::uint64_t>(frames[index]) == caller)
            {
                first = index;
                break;
            }
        }

        if (first == captured)
        {
            first = 0;
            while (first < captured && store.IsOwnAddress(reinterpret_cast<std::uint64_t>(frames[first]))) ++first;
        }

        PovProbeEvent event{};
        event.Kind = kind;
        event.ThreadId = ::GetCurrentThreadId();
        event.Caller = caller;
        event.Args = { window, name, a4, a5 };
        event.Result = result;
        for (std::size_t index{ first }; index < captured && event.ExtraCount < k_PovProbeExtraWords; ++index)
        {
            event.Extra[event.ExtraCount++] = reinterpret_cast<std::uint64_t>(frames[index]);
        }

        store.Record(event);

        // A creator can return the address of a screen that was destroyed: its next update is a new screen, not a change of the old one.
        if (result != 0 && event.ThreadId == store.GetGameThread()) store.ForgetScreen(result);
    }
}

namespace Engine::Base::Pov::Hook
{
    // note: The eight hooked functions below follow one rule: the in flight scope spans the call of the original, the original is always called
    // when there is one (the statics are cleared only after the hook is removed and drained), and nothing here allocates, logs or waits. The
    // window is the one of the other detours of the project (ObserverCameraDetour): a thread that read the original pointer after Uninstall
    // cleared it returns without calling it, and a thread between the jump into the detour and its in flight scope is not counted by the drain
    // (a few instructions). Uninstall disables the hooks first, so only threads already past the jump can meet either case.
    //
    // All eight forward eight integer arguments and the return value, so the original receives exactly what the game passed: a function with up
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
            NoteCall(self->m_Store, PovProbeKind::ViewPlayer, reinterpret_cast<std::uint64_t>(_ReturnAddress()), static_cast<std::uint32_t>(a0),
                static_cast<std::uint32_t>(a1), 0, 0);
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
            NoteCall(self->m_Store, PovProbeKind::ViewUnit, reinterpret_cast<std::uint64_t>(_ReturnAddress()), static_cast<std::uint32_t>(a0),
                static_cast<std::uint32_t>(a1), 0, 0);
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
            NoteCall(self->m_Store, PovProbeKind::PlayerUnit, reinterpret_cast<std::uint64_t>(_ReturnAddress()), static_cast<std::uint32_t>(a0),
                static_cast<std::uint32_t>(a1), 0, 0);
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
            NoteCall(self->m_Store, PovProbeKind::PlayerAssignUnit, reinterpret_cast<std::uint64_t>(_ReturnAddress()), a0, a1, a2, a3);
        }

        return original(a0, a1, a2, a3, a4, a5, a6, a7);
    }

    // note: The creators of screens return the screen. The arguments after the context are window (int32), name and a4, and only 0x2CA014 has a
    // fifth one (a5). The call is noted after the original returned, so the screen is known. The original pointers are cleared by Uninstall after
    // the hooks are removed and the calls drained, so they are the last thing to go. The only window left is the one of every detour of the
    // project: a thread that read the pointer after it was cleared returns 0 (no screen) without calling the original.
    auto __fastcall PovProbeDetours::HookedScreenCreate(std::uint64_t a0, std::uint64_t a1, std::uint64_t a2, std::uint64_t a3, std::uint64_t a4,
        std::uint64_t a5, std::uint64_t a6, std::uint64_t a7) -> std::uint64_t
    {
        Engine::Base::Hook::System::InFlightScope scope{ s_InFlight };

        const PovProbeWideFunction original{ s_ScreenCreateOriginal };
        if (original == nullptr) return 0;

        const std::uint64_t caller{ reinterpret_cast<std::uint64_t>(_ReturnAddress()) };
        const std::uint64_t result{ original(a0, a1, a2, a3, a4, a5, a6, a7) };

        PovProbeDetours* const self{ s_Instance.load() };
        if (self != nullptr && self->m_Store.IsActive())
        {
            NoteCreate(self->m_Store, PovProbeKind::ScreenCreate, caller, static_cast<std::uint32_t>(a1), static_cast<std::uint32_t>(a2),
                static_cast<std::uint32_t>(a3), 0, result);
        }

        return result;
    }

    auto __fastcall PovProbeDetours::HookedScreenCreateAlt(std::uint64_t a0, std::uint64_t a1, std::uint64_t a2, std::uint64_t a3, std::uint64_t a4,
        std::uint64_t a5, std::uint64_t a6, std::uint64_t a7) -> std::uint64_t
    {
        Engine::Base::Hook::System::InFlightScope scope{ s_InFlight };

        const PovProbeWideFunction original{ s_ScreenCreateAltOriginal };
        if (original == nullptr) return 0;

        const std::uint64_t caller{ reinterpret_cast<std::uint64_t>(_ReturnAddress()) };
        const std::uint64_t result{ original(a0, a1, a2, a3, a4, a5, a6, a7) };

        PovProbeDetours* const self{ s_Instance.load() };
        if (self != nullptr && self->m_Store.IsActive())
        {
            NoteCreate(self->m_Store, PovProbeKind::ScreenCreateAlt, caller, static_cast<std::uint32_t>(a1), static_cast<std::uint32_t>(a2),
                static_cast<std::uint32_t>(a3), 0, result);
        }

        return result;
    }

    auto __fastcall PovProbeDetours::HookedScreenCreateEx(std::uint64_t a0, std::uint64_t a1, std::uint64_t a2, std::uint64_t a3, std::uint64_t a4,
        std::uint64_t a5, std::uint64_t a6, std::uint64_t a7) -> std::uint64_t
    {
        Engine::Base::Hook::System::InFlightScope scope{ s_InFlight };

        const PovProbeWideFunction original{ s_ScreenCreateExOriginal };
        if (original == nullptr) return 0;

        const std::uint64_t caller{ reinterpret_cast<std::uint64_t>(_ReturnAddress()) };
        const std::uint64_t result{ original(a0, a1, a2, a3, a4, a5, a6, a7) };

        PovProbeDetours* const self{ s_Instance.load() };
        if (self != nullptr && self->m_Store.IsActive())
        {
            NoteCreate(self->m_Store, PovProbeKind::ScreenCreateEx, caller, static_cast<std::uint32_t>(a1), static_cast<std::uint32_t>(a2),
                static_cast<std::uint32_t>(a3), static_cast<std::uint32_t>(a4), result);
        }

        return result;
    }

    // note: Same forwarding as PlayerAssignUnit. The first value is read before the call (the function may change it) and guarded.
    auto __fastcall PovProbeDetours::HookedCursor(std::uint64_t thisPointer, std::uint64_t valuePointer, std::uint64_t a2, std::uint64_t a3,
        std::uint64_t a4, std::uint64_t a5, std::uint64_t a6, std::uint64_t a7) -> std::uint64_t
    {
        Engine::Base::Hook::System::InFlightScope scope{ s_InFlight };

        const PovProbeWideFunction original{ s_CursorOriginal };
        if (original == nullptr) return 0;

        PovProbeDetours* const self{ s_Instance.load() };
        bool isFollowed{ false };
        if (self != nullptr && self->m_Store.IsActive())
        {
            self->m_Store.RecordCursor(thisPointer, ReadFirstValue(valuePointer));

            // The fields of the screen at the start of its update. Only the game thread is followed, the others are counted.
            if (::GetCurrentThreadId() == self->m_Store.GetGameThread())
            {
                isFollowed = true;
                PovProbeScreen screen{};
                if (ReadScreen(thisPointer, screen)) self->m_Store.NoteScreen(thisPointer, screen, ::GetCurrentThreadId(), false);
            }
            else
            {
                self->m_Store.CountOffThread();
            }
        }

        const std::uint64_t result{ original(thisPointer, valuePointer, a2, a3, a4, a5, a6, a7) };

        // The same fields when the update returned, so a close set by this update is seen in this same call. The in flight scope still covers it,
        // so the instance is there. The screen was valid when the update began: a read of memory the engine freed in between gives garbage, not a fault.
        if (isFollowed)
        {
            PovProbeScreen screen{};
            if (ReadScreen(thisPointer, screen)) self->m_Store.NoteScreen(thisPointer, screen, ::GetCurrentThreadId(), true);
        }

        return result;
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
            { &k_ScreenCreatePrologue, k_ScreenCreateRva, "2C9EFC", reinterpret_cast<void*>(&HookedScreenCreate),
                reinterpret_cast<void**>(&s_ScreenCreateOriginal) },
            { &k_ScreenCreatePrologue, k_ScreenCreateAltRva, "2C9F88", reinterpret_cast<void*>(&HookedScreenCreateAlt),
                reinterpret_cast<void**>(&s_ScreenCreateAltOriginal) },
            { &k_ScreenCreateExPrologue, k_ScreenCreateExRva, "2CA014", reinterpret_cast<void*>(&HookedScreenCreateEx),
                reinterpret_cast<void**>(&s_ScreenCreateExOriginal) },
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
        s_ScreenCreateOriginal = nullptr;
        s_ScreenCreateAltOriginal = nullptr;
        s_ScreenCreateExOriginal = nullptr;
        s_Instance.store(nullptr);
        m_IsInstalled.store(false);
    }
}