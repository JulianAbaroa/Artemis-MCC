module Engine.Render.Hook;
import :Chud;

import Engine.Base.Hook.System;
import std;

namespace
{
    using Engine::Render::State::RenderStore;
    using Engine::Render::Type::k_IdleTimeout;

    using Engine::Base::Memory::Type::Signature::k_ChudDraw3DPrologue;
    using Engine::Base::Memory::Type::Signature::k_ChudDraw3DRva;
    using Engine::Base::Memory::Type::Signature::k_ChudDrawLatePrologue;
    using Engine::Base::Memory::Type::Signature::k_ChudDrawLateRva;
    using Engine::Base::Memory::Type::Signature::k_ChudDrawPrologue;
    using Engine::Base::Memory::Type::Signature::k_ChudDrawRva;
    using Engine::Base::Memory::Type::Signature::k_ChudWindow;
}

namespace Engine::Render::Hook
{
    // note: The scope of every hooked pass spans the call of the original, so WaitForIdle also covers the threads inside the trampoline.
    auto __fastcall ChudDetour::HookedDraw(std::uint32_t user) -> void
    {
        ChudDetour* self{ s_Instance.load() };

        if (self == nullptr)
        {
            if (s_OriginalDraw != nullptr) s_OriginalDraw(user);
            return;
        }

        RenderStore::HookScope scope{ self->m_RenderStore };

        if (self->m_RenderStore.IsGameHudHidden()) return;

        if (s_OriginalDraw != nullptr) s_OriginalDraw(user);
    }

    auto __fastcall ChudDetour::HookedDrawLate(std::uint32_t user) -> void
    {
        ChudDetour* self{ s_Instance.load() };

        if (self == nullptr)
        {
            if (s_OriginalDrawLate != nullptr) s_OriginalDrawLate(user);
            return;
        }

        RenderStore::HookScope scope{ self->m_RenderStore };

        if (self->m_RenderStore.IsGameHudHidden()) return;

        if (s_OriginalDrawLate != nullptr) s_OriginalDrawLate(user);
    }

    auto __fastcall ChudDetour::HookedDraw3D(std::uint32_t user) -> void
    {
        ChudDetour* self{ s_Instance.load() };

        if (self == nullptr)
        {
            if (s_OriginalDraw3D != nullptr) s_OriginalDraw3D(user);
            return;
        }

        RenderStore::HookScope scope{ self->m_RenderStore };

        if (self->m_RenderStore.IsGameHudHidden()) return;

        if (s_OriginalDraw3D != nullptr) s_OriginalDraw3D(user);
    }

    auto ChudDetour::InstallOne(const char* name, const Signature& prologue, std::uint32_t rva, void* hooked, void** original,
        std::atomic<void*>& address) -> bool
    {
        // The window is 0: the prologue must begin exactly at the RVA.
        void* functionAddress{ reinterpret_cast<void*>(m_AOBService.FindFunctionAtRva(prologue, rva, k_ChudWindow)) };

        if (functionAddress == nullptr)
        {
            m_Log.Message("[ChudDetour] ERROR: {} not found at rva {:X}, its hook is not installed.", name, rva);
            return false;
        }

        if (!Engine::Base::Hook::System::InstallDetour(functionAddress, hooked, original, "[ChudDetour]", m_Log))
        {
            return false;
        }

        address.store(functionAddress);
        m_Log.Message("[ChudDetour] INFO: {} hooked at rva {:X}.", name, rva);
        return true;
    }

    auto ChudDetour::Install() -> bool
    {
        const std::lock_guard<std::mutex> lock{ m_Mutex };

        if (m_IsInstalled.load()) return m_DrawAddress.load() != nullptr;
        s_Instance.store(this);

        const bool isDrawHooked{ this->InstallOne("chud draw", k_ChudDrawPrologue, k_ChudDrawRva,
            reinterpret_cast<void*>(&HookedDraw), reinterpret_cast<void**>(&s_OriginalDraw), m_DrawAddress) };
        const bool isLateHooked{ this->InstallOne("chud draw late", k_ChudDrawLatePrologue, k_ChudDrawLateRva,
            reinterpret_cast<void*>(&HookedDrawLate), reinterpret_cast<void**>(&s_OriginalDrawLate), m_DrawLateAddress) };
        const bool is3DHooked{ this->InstallOne("chud draw 3D", k_ChudDraw3DPrologue, k_ChudDraw3DRva,
            reinterpret_cast<void*>(&HookedDraw3D), reinterpret_cast<void**>(&s_OriginalDraw3D), m_Draw3DAddress) };

        if (!isDrawHooked && !isLateHooked && !is3DHooked)
        {
            s_Instance.store(nullptr);
            return false;
        }

        m_IsInstalled.store(true);
        return isDrawHooked;
    }

    auto ChudDetour::Disable(const std::atomic<void*>& address) -> void
    {
        void* functionAddress{ address.load() };
        if (functionAddress == nullptr) return;

        Engine::Base::Hook::System::DisableDetour(functionAddress);
    }

    auto ChudDetour::Remove(std::atomic<void*>& address, const char* name) -> void
    {
        void* functionAddress{ address.exchange(nullptr) };
        if (functionAddress == nullptr) return;

        m_Log.Message("[ChudDetour] INFO: Removing the hook of {}.", name);
        Engine::Base::Hook::System::RemoveDetour(functionAddress, "[ChudDetour]", m_Log);
    }

    auto ChudDetour::Uninstall() -> void
    {
        const std::lock_guard<std::mutex> lock{ m_Mutex };

        if (!m_IsInstalled.load()) return;

        this->Disable(m_DrawAddress);
        this->Disable(m_DrawLateAddress);
        this->Disable(m_Draw3DAddress);

        if (!m_RenderService.WaitForIdle(k_IdleTimeout))
        {
            m_Log.Message("[ChudDetour] WARNING: A call was still running when the hooks were removed.");
        }

        this->Remove(m_DrawAddress, "chud draw");
        this->Remove(m_DrawLateAddress, "chud draw late");
        this->Remove(m_Draw3DAddress, "chud draw 3D");

        s_OriginalDraw = nullptr;
        s_OriginalDrawLate = nullptr;
        s_OriginalDraw3D = nullptr;

        m_IsInstalled.store(false);
        s_Instance.store(nullptr);
    }
}