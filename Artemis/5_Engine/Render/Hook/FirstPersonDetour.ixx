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
    // Hooks the first person model pass, the game's first person unit test, the first person depth pass (D331) and the forward pass of the view. The model pass is skipped for user 0 with the render
    // flag HideFirstPerson or while Drive moves the camera. While Drive moves the camera the unit test answers false for user 0, so the
    // game draws the player's own biped and the third person effects. Render only: the pose of the director is never written.
    // The model pass is also measured (VW.6.y.2): for user 0 each call is counted by its caller (the function that called it, by the
    // return address) and, once a probe is armed in the store, the render targets, depth stencil, viewport, depth stencil state and
    // rasterizer state bound at that moment are copied to the store before the original runs. Nothing changes what is drawn.
    // The pass 0x26DA08 (void(void* view, int32 pass): first person camera, queue the model, draw the list, normal camera, pop) never changes
    // what is drawn. Its call with pass 0xB is the one that writes the depth of the arms and weapon: at its entry and exit the render service
    // copies the depth for the overlay mask (VW.6.y.2 B) and, with a probe armed, for the depth pairs (A5, the pairs note the pass argument).
    // The forward pass 0x26EA78 (void(void* view, uint8 phase)) is only measured (VW.6.y.2 D): with a probe armed the render service copies the
    // color of the render target 0 at its entry and exit (the color pairs), to see whether it draws the parts of the first person model that
    // the depth pass does not. Nothing changes what is drawn.
    // The opaque color pass 0x26E500 (void(void* view), once per view, after the first person lists of 0x26EA78 phase 0) is hooked to draw the overlay
    // in the scene (VW.6.z.1, D334): the detour calls the original and then RenderService::OnSceneDraw, so the overlay is over the opaque world and
    // under the effects, the blade and the HUD of the game.
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
        using FirstPersonDepthPassFunction = Engine::Render::Type::FirstPersonDepthPassFunction;
        using FirstPersonForwardFunction = Engine::Render::Type::FirstPersonForwardFunction;
        using OpaqueColorPassFunction = Engine::Render::Type::OpaqueColorPassFunction;
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
        // return: True if at least one of the five is hooked.
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
        static inline FirstPersonDepthPassFunction s_OriginalDepthPass{ nullptr };
        static inline FirstPersonForwardFunction s_OriginalForward{ nullptr };
        static inline OpaqueColorPassFunction s_OriginalOpaque{ nullptr };

        // Base of the game module, found when the hooks are installed (the address of a hooked function minus its RVA). 0 when not installed.
        std::atomic<std::uintptr_t> m_ModuleBase{ 0 };
        std::atomic<void*> m_ModelAddress{ nullptr };
        std::atomic<void*> m_UnitAddress{ nullptr };
        std::atomic<void*> m_DepthPassAddress{ nullptr };
        std::atomic<void*> m_ForwardAddress{ nullptr };
        std::atomic<void*> m_OpaqueAddress{ nullptr };
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
        static auto __fastcall HookedDepthPass(void* view, std::int32_t pass) -> void;
        static auto __fastcall HookedForward(void* view, std::uint8_t phase) -> void;
        static auto __fastcall HookedOpaque(void* view) -> void;
        static auto __fastcall HookedUnit(std::uint32_t user, std::uint32_t objectHandle) -> bool;
    };
}