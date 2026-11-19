module;

#include <windows.h>

module Engine.Render.Hook;
import :FirstPerson;

import Engine.Base.Hook.System;
import std;

namespace
{
    using Engine::Render::State::RenderStore;
    using Engine::Render::Type::MaskPairKind;
    using Engine::Render::Type::k_IdleTimeout;

    using Engine::Base::Memory::Type::ObserverCamera::k_OverrideMaxAge;
    using Engine::Base::Memory::Type::Signature::k_FirstPersonDepthPassPrologue;
    using Engine::Base::Memory::Type::Signature::k_FirstPersonDepthPassRva;
    using Engine::Base::Memory::Type::Signature::k_FirstPersonForwardPrologue;
    using Engine::Base::Memory::Type::Signature::k_FirstPersonForwardRva;
    using Engine::Base::Memory::Type::Signature::k_FirstPersonModelPrologue;
    using Engine::Base::Memory::Type::Signature::k_FirstPersonModelRva;
    using Engine::Base::Memory::Type::Signature::k_FirstPersonWindow;
    using Engine::Base::Memory::Type::Signature::k_IsFirstPersonUnitPrologue;
    using Engine::Base::Memory::Type::Signature::k_IsFirstPersonUnitRva;
    using Engine::Base::Memory::Type::Signature::k_OpaqueColorPassPrologue;
    using Engine::Base::Memory::Type::Signature::k_OpaqueColorPassRva;

    // The first local player, the only user that is ever changed.
    constexpr std::uint32_t k_LocalUser{ 0 };

    // Time between disabling the hooks and draining them (see Uninstall).
    constexpr std::chrono::milliseconds k_DisableGrace{ 20 };

}

namespace Engine::Render::Hook
{
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

        if (FirstPersonDetour::IsFirstPersonModelSkipped(user, self->m_RenderStore.IsFirstPersonHidden(), self->IsDriveActive())) return;

        if (s_OriginalModel != nullptr) s_OriginalModel(camera, user, pass);
    }

    // note: The service decides if this call is copied for the overlay mask (VW.6.y.2 B, E: the first call with pass 0xB of the frame and the
    // first with pass 0, the game draws the arms in one or the other). Without a mask request it only reads atomics. The scope spans the
    // original, as in the other hooks.
    auto __fastcall FirstPersonDetour::HookedDepthPass(void* view, std::int32_t pass) -> void
    {
        FirstPersonDetour* self{ s_Instance.load() };

        if (self == nullptr)
        {
            if (s_OriginalDepthPass != nullptr) s_OriginalDepthPass(view, pass);
            return;
        }

        RenderStore::HookScope scope{ self->m_RenderStore };

        const std::optional<MaskPairKind> maskPair{ self->m_RenderService.BeginMaskCopy(pass) };

        if (s_OriginalDepthPass != nullptr) s_OriginalDepthPass(view, pass);

        if (maskPair) self->m_RenderService.EndMaskCopy(*maskPair);
    }

    // note: Calls the original untouched. The scope spans the original, as in the other hooks.
    auto __fastcall FirstPersonDetour::HookedForward(void* view, std::uint8_t phase) -> void
    {
        FirstPersonDetour* self{ s_Instance.load() };

        if (self == nullptr)
        {
            if (s_OriginalForward != nullptr) s_OriginalForward(view, phase);
            return;
        }

        RenderStore::HookScope scope{ self->m_RenderStore };

        if (s_OriginalForward != nullptr) s_OriginalForward(view, phase);
    }

    // note: The overlay draws here (VW.6.z.1, D334): at the exit of the opaque color pass of the view, so it is over the opaque world and under the
    // effects, blade, shields and HUD of the game. The original runs first. OnSceneDraw is catch-all and takes the one scene point of the frame.
    auto __fastcall FirstPersonDetour::HookedOpaque(void* view) -> void
    {
        FirstPersonDetour* self{ s_Instance.load() };

        if (self == nullptr)
        {
            if (s_OriginalOpaque != nullptr) s_OriginalOpaque(view);
            return;
        }

        RenderStore::HookScope scope{ self->m_RenderStore };

        if (s_OriginalOpaque != nullptr) s_OriginalOpaque(view);

        self->m_RenderService.OnSceneDraw();
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

        if (!FirstPersonDetour::IsThirdPersonShown(user, self->IsDriveActive(), original))
        {
            self->m_RenderStore.CountFirstPersonUnit();
            return original;
        }

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
        m_Log.Message("[FirstPersonDetour] INFO: {} hooked at rva {:X}.", name, rva);
        return true;
    }

    auto FirstPersonDetour::Install() -> bool
    {
        const std::lock_guard<std::mutex> lock{ m_Mutex };

        if (m_IsInstalled.load()) return m_ModelAddress.load() != nullptr || m_UnitAddress.load() != nullptr
            || m_DepthPassAddress.load() != nullptr || m_ForwardAddress.load() != nullptr || m_OpaqueAddress.load() != nullptr;
        s_Instance.store(this);

        const bool isModelHooked{ this->InstallOne("first person model", k_FirstPersonModelPrologue, k_FirstPersonModelRva,
            reinterpret_cast<void*>(&HookedModel), reinterpret_cast<void**>(&s_OriginalModel), m_ModelAddress) };
        const bool isUnitHooked{ this->InstallOne("is first person unit", k_IsFirstPersonUnitPrologue, k_IsFirstPersonUnitRva,
            reinterpret_cast<void*>(&HookedUnit), reinterpret_cast<void**>(&s_OriginalUnit), m_UnitAddress) };

        const bool isDepthPassHooked{ this->InstallOne("first person depth pass", k_FirstPersonDepthPassPrologue, k_FirstPersonDepthPassRva,
            reinterpret_cast<void*>(&HookedDepthPass), reinterpret_cast<void**>(&s_OriginalDepthPass), m_DepthPassAddress) };

        const bool isForwardHooked{ this->InstallOne("first person forward", k_FirstPersonForwardPrologue, k_FirstPersonForwardRva,
            reinterpret_cast<void*>(&HookedForward), reinterpret_cast<void**>(&s_OriginalForward), m_ForwardAddress) };

        const bool isOpaqueHooked{ this->InstallOne("opaque color pass", k_OpaqueColorPassPrologue, k_OpaqueColorPassRva,
            reinterpret_cast<void*>(&HookedOpaque), reinterpret_cast<void**>(&s_OriginalOpaque), m_OpaqueAddress) };

        if (!isModelHooked && !isUnitHooked && !isDepthPassHooked && !isForwardHooked && !isOpaqueHooked)
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
        m_RenderStore.SetFirstPersonHooksInstalled(isModelHooked, isUnitHooked, isDepthPassHooked, isForwardHooked);
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

        m_RenderStore.SetFirstPersonHooksInstalled(false, false, false, false);

        this->Disable(m_ModelAddress);
        this->Disable(m_UnitAddress);
        this->Disable(m_DepthPassAddress);
        this->Disable(m_ForwardAddress);
        this->Disable(m_OpaqueAddress);

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
        this->Remove(m_DepthPassAddress, "first person depth pass");
        this->Remove(m_ForwardAddress, "first person forward");
        this->Remove(m_OpaqueAddress, "opaque color pass");

        s_OriginalModel = nullptr;
        s_OriginalUnit = nullptr;
        s_OriginalDepthPass = nullptr;
        s_OriginalForward = nullptr;
        s_OriginalOpaque = nullptr;

        m_IsInstalled.store(false);
        s_Instance.store(nullptr);
    }
}