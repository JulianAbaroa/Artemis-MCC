export module Engine.Render.Hook:FirstPerson;

import Common.Log.System;
import Engine.Base.Memory.Type;
import Engine.Base.Memory.State;
import Engine.Base.Memory.System;
import Engine.Render.Type;
import Engine.Render.State;
import Engine.Render.System;
import std;

export namespace Engine::Render::Hook
{
    // Hooks the first person model pass, the game's first person unit test and the first person G-buffer pass (D331). The model pass is skipped for user 0 with the render
    // flag HideFirstPerson or while Drive moves the camera. While Drive moves the camera the unit test answers false for user 0, so the
    // game draws the player's own biped and the third person effects. Render only: the pose of the director is never written.
    // The model pass is also measured (VW.6.y.2): for user 0 each call is counted by its caller (the function that called it, by the
    // return address) and, once a probe is armed in the store, the render targets, depth stencil, viewport, depth stencil state and
    // rasterizer state bound at that moment are copied to the store before the original runs. Nothing changes what is drawn.
    // The G-buffer pass 0x26E2A0 (void(void* view): first person camera, queue the model, draw the list, normal camera, second layer) is
    // hooked only to measure: with a probe armed the depth is copied at its entry and at its exit (VW.6.y.2 A4), otherwise it calls the original.
    // note: The unit test is also called by effects and perhaps from another thread than the render. Both detours read only atomics and the
    // CameraOverrideStore without locking, and do not allocate or log.
    class FirstPersonDetour
    {
    private:
        using Log = Common::Log::System::Log;

        using Signature = Engine::Base::Memory::Type::Signature::Signature;

        using CameraOverrideStore = Engine::Base::Memory::State::CameraOverrideStore;

        using AOBService = Engine::Base::Memory::System::AOBService;

        using FirstPersonCaller = Engine::Render::Type::FirstPersonCaller;
        using FirstPersonModelFunction = Engine::Render::Type::FirstPersonModelFunction;
        using FirstPersonPassFunction = Engine::Render::Type::FirstPersonPassFunction;
        using IsFirstPersonUnitFunction = Engine::Render::Type::IsFirstPersonUnitFunction;

        using RenderStore = Engine::Render::State::RenderStore;

        using RenderService = Engine::Render::System::RenderService;

    public:
        FirstPersonDetour(Log& log, AOBService& aobService, RenderStore& renderStore, RenderService& renderService,
            CameraOverrideStore& overrideStore) :
            m_Log(log), m_AOBService(aobService), m_RenderStore(renderStore), m_RenderService(renderService), m_OverrideStore(overrideStore) {}
        ~FirstPersonDetour() = default;

        // Finds each function by its RVA and prologue and hooks it. One whose prologue does not match is logged as an ERROR and left alone,
        // the others are still hooked. Does nothing if already installed. Can be called again after Uninstall.
        // return: True if at least one of the three is hooked.
        auto Install() -> bool;

        // Waits for the calls in flight to drain, then removes the hooks that were installed. Does nothing if not installed.
        // note: Can be called from two threads (the unhook of the game thread and the shutdown of the main thread), serialized by a mutex.
        auto Uninstall() -> void;

        // return: Which function of the game called the model pass, by the return address minus the base of the game module. Other when the
        // address is in none of the known functions.
        static auto ClassifyCaller(std::uint32_t rva) -> FirstPersonCaller;

        // return: Whether the model pass is skipped. Only user 0 is ever skipped.
        static auto IsFirstPersonModelSkipped(std::uint32_t user, bool isHidden, bool isDriveActive) -> bool;

        // param original: What the game answered.
        // return: Whether the answer is turned to false: user 0, Drive active and the game said true.
        static auto IsThirdPersonShown(std::uint32_t user, bool isDriveActive, bool original) -> bool;

    private:
        Log& m_Log;
        AOBService& m_AOBService;
        RenderStore& m_RenderStore;
        RenderService& m_RenderService;
        CameraOverrideStore& m_OverrideStore;

        static inline std::atomic<FirstPersonDetour*> s_Instance{ nullptr };
        static inline FirstPersonModelFunction s_OriginalModel{ nullptr };
        static inline IsFirstPersonUnitFunction s_OriginalUnit{ nullptr };
        static inline FirstPersonPassFunction s_OriginalPass{ nullptr };

        // Base of the game module, found when the hooks are installed (the address of a hooked function minus its RVA). 0 when not installed.
        std::atomic<std::uintptr_t> m_ModuleBase{ 0 };
        std::atomic<void*> m_ModelAddress{ nullptr };
        std::atomic<void*> m_UnitAddress{ nullptr };
        std::atomic<void*> m_PassAddress{ nullptr };
        std::atomic<bool> m_IsInstalled{ false };
        std::mutex m_Mutex{};

        // return: True if the function was hooked.
        auto InstallOne(const char* name, const Signature& prologue, std::uint32_t rva, void* hooked, void** original,
            std::atomic<void*>& address) -> bool;

        // Disables the hook at the address. Does nothing for a null address.
        auto Disable(const std::atomic<void*>& address) -> void;

        auto Remove(std::atomic<void*>& address, const char* name) -> void;

        // Counts the call for its caller and, while the probe is armed, measures the D3D state. User 0 only.
        // param returnAddress: The return address of the detour.
        auto NoteModelCall(const void* returnAddress, std::uint8_t pass) -> void;

        // return: Whether a valid and fresh pose is published, the same rule the observer camera hook applies.
        auto IsDriveActive() const -> bool;

        static auto __fastcall HookedModel(void* camera, std::uint32_t user, std::uint8_t pass) -> void;
        static auto __fastcall HookedPass(void* view) -> void;
        static auto __fastcall HookedUnit(std::uint32_t user, std::uint32_t objectHandle) -> bool;
    };
}