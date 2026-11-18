module;

#include <intrin.h>

module Engine.Render.Hook;
import :FirstPerson;

import Engine.Base.Hook.System;
import std;

namespace
{
    using Engine::Render::State::RenderStore;
    using Engine::Render::Type::k_FirstPersonCallerRanges;
    using Engine::Render::Type::k_IdleTimeout;

    using Engine::Base::Memory::Type::ObserverCamera::k_OverrideMaxAge;
    using Engine::Base::Memory::Type::Signature::k_FirstPersonModelPrologue;
    using Engine::Base::Memory::Type::Signature::k_FirstPersonModelRva;
    using Engine::Base::Memory::Type::Signature::k_FirstPersonWindow;
    using Engine::Base::Memory::Type::Signature::k_IsFirstPersonUnitPrologue;
    using Engine::Base::Memory::Type::Signature::k_IsFirstPersonUnitRva;

    // The first local player, the only user that is ever changed.
    constexpr std::uint32_t k_LocalUser{ 0 };

    // Time between disabling the hooks and draining them (see Uninstall).
    constexpr std::chrono::milliseconds k_DisableGrace{ 20 };
}

namespace Engine::Render::Hook
{
    auto FirstPersonDetour::ClassifyCaller(std::uint32_t rva) -> FirstPersonCaller
    {
        for (const auto& range : k_FirstPersonCallerRanges)
        {
            if (rva >= range.Rva && rva - range.Rva < range.Size) return range.Caller;
        }

        return FirstPersonCaller::Other;
    }

    auto FirstPersonDetour::NoteModelCall(const void* returnAddress, std::uint8_t pass) -> void
    {
        const std::uintptr_t base{ m_ModuleBase.load() };
        const std::uintptr_t address{ reinterpret_cast<std::uintptr_t>(returnAddress) };
        const std::uint32_t rva{ base != 0 && address >= base ? static_cast<std::uint32_t>(address - base) : 0 };
        const FirstPersonCaller caller{ FirstPersonDetour::ClassifyCaller(rva) };

        m_RenderStore.CountFirstPersonCall(caller);

        if (!m_RenderStore.IsFirstPersonProbeArmed()) return;

        m_RenderStore.SetFirstPersonDriveActive(this->IsDriveActive());
        m_RenderService.MeasureFirstPerson(caller, rva, pass);
    }

    auto FirstPersonDetour::IsFirstPersonModelSkipped(std::uint32_t user, bool isHidden, bool isDriveActive) -> bool
    {
        return user == k_LocalUser && (isHidden || isDriveActive);
    }

    auto FirstPersonDetour::IsThirdPersonShown(std::uint32_t user, bool isDriveActive, bool original) -> bool
    {
        return user == k_LocalUser && isDriveActive && original;
    }

    auto FirstPersonDetour::IsDriveActive() const -> bool
    {
        return m_OverrideStore.Read(k_OverrideMaxAge).has_value();
    }

    // note: The scope spans the call of the original, so WaitForIdle also covers the threads inside the trampoline.
    auto __fastcall FirstPersonDetour::HookedModel(void* camera, std::uint32_t user, std::uint8_t pass) -> void
    {
        FirstPersonDetour* self{ s_Instance.load() };

        if (self == nullptr)
        {
            if (s_OriginalModel != nullptr) s_OriginalModel(camera, user, pass);
            return;
        }

        RenderStore::HookScope scope{ self->m_RenderStore };

        if (user == k_LocalUser) self->NoteModelCall(_ReturnAddress(), pass);

        if (FirstPersonDetour::IsFirstPersonModelSkipped(user, self->m_RenderStore.IsFirstPersonHidden(), self->IsDriveActive())) return;

        if (s_OriginalModel != nullptr) s_OriginalModel(camera, user, pass);
    }

    auto __fastcall FirstPersonDetour::HookedUnit(std::uint32_t user, std::uint32_t objectHandle) -> bool
    {
        FirstPersonDetour* self{ s_Instance.load() };

        if (self == nullptr)
        {
            return s_OriginalUnit != nullptr && s_OriginalUnit(user, objectHandle);
        }

        RenderStore::HookScope scope{ self->m_RenderStore };

        const bool original{ s_OriginalUnit != nullptr && s_OriginalUnit(user, objectHandle) };

        // Users other than 0 never reach the store.
        if (user != k_LocalUser || !original) return original;

        if (!FirstPersonDetour::IsThirdPersonShown(user, self->IsDriveActive(), original)) return original;

        self->m_RenderStore.CountThirdPerson();
        return false;
    }

    auto FirstPersonDetour::InstallOne(const char* name, const Signature& prologue, std::uint32_t rva, void* hooked, void** original,
        std::atomic<void*>& address) -> bool
    {
        // The window is 0: the prologue must begin exactly at the RVA.
        void* functionAddress{ reinterpret_cast<void*>(m_AOBService.FindFunctionAtRva(prologue, rva, k_FirstPersonWindow)) };

        if (functionAddress == nullptr)
        {
            m_Log.Message("[FirstPersonDetour] ERROR: {} not found at rva {:X}, its hook is not installed.", name, rva);
            return false;
        }

        if (!Engine::Base::Hook::System::InstallDetour(functionAddress, hooked, original, "[FirstPersonDetour]", m_Log))
        {
            return false;
        }

        address.store(functionAddress);
        m_ModuleBase.store(reinterpret_cast<std::uintptr_t>(functionAddress) - rva);
        m_Log.Message("[FirstPersonDetour] INFO: {} hooked at rva {:X}.", name, rva);
        return true;
    }

    auto FirstPersonDetour::Install() -> bool
    {
        const std::lock_guard<std::mutex> lock{ m_Mutex };

        if (m_IsInstalled.load()) return m_ModelAddress.load() != nullptr || m_UnitAddress.load() != nullptr;
        s_Instance.store(this);

        const bool isModelHooked{ this->InstallOne("first person model", k_FirstPersonModelPrologue, k_FirstPersonModelRva,
            reinterpret_cast<void*>(&HookedModel), reinterpret_cast<void**>(&s_OriginalModel), m_ModelAddress) };
        const bool isUnitHooked{ this->InstallOne("is first person unit", k_IsFirstPersonUnitPrologue, k_IsFirstPersonUnitRva,
            reinterpret_cast<void*>(&HookedUnit), reinterpret_cast<void**>(&s_OriginalUnit), m_UnitAddress) };

        if (!isModelHooked && !isUnitHooked)
        {
            s_Instance.store(nullptr);
            return false;
        }

        if (!isModelHooked || !isUnitHooked)
        {
            // Half of the pair: with Drive the arms are hidden but the body is not drawn, or the other way round.
            m_Log.Message("[FirstPersonDetour] WARNING: Only one of the two hooks is installed, Drive will not show the player right.");
        }

        m_IsInstalled.store(true);
        m_RenderStore.SetFirstPersonHooksInstalled(isModelHooked, isUnitHooked);
        return true;
    }

    auto FirstPersonDetour::Disable(const std::atomic<void*>& address) -> void
    {
        void* functionAddress{ address.load() };
        if (functionAddress == nullptr) return;

        Engine::Base::Hook::System::DisableDetour(functionAddress);
    }

    auto FirstPersonDetour::Remove(std::atomic<void*>& address, const char* name) -> void
    {
        void* functionAddress{ address.exchange(nullptr) };
        if (functionAddress == nullptr) return;

        m_Log.Message("[FirstPersonDetour] INFO: Removing the hook of {}.", name);
        Engine::Base::Hook::System::RemoveDetour(functionAddress, "[FirstPersonDetour]", m_Log);
    }

    auto FirstPersonDetour::Uninstall() -> void
    {
        const std::lock_guard<std::mutex> lock{ m_Mutex };

        if (!m_IsInstalled.load()) return;

        m_RenderStore.SetFirstPersonHooksInstalled(false, false);

        this->Disable(m_ModelAddress);
        this->Disable(m_UnitAddress);

        // note: A thread can be past the jump of the hook but not yet inside its HookScope, where WaitForIdle does not see it.
        // 0x266104 runs per object and per frame, maybe on several threads, so that window is real: the grace lets those threads
        // reach the scope before the drain. Uninstall only runs at unhook and shutdown.
        std::this_thread::sleep_for(k_DisableGrace);

        if (!m_RenderService.WaitForIdle(k_IdleTimeout))
        {
            m_Log.Message("[FirstPersonDetour] WARNING: A call was still running when the hooks were removed.");
        }

        this->Remove(m_ModelAddress, "first person model");
        this->Remove(m_UnitAddress, "is first person unit");

        s_OriginalModel = nullptr;
        s_OriginalUnit = nullptr;

        m_ModuleBase.store(0);
        m_IsInstalled.store(false);
        s_Instance.store(nullptr);
    }
}