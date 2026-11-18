module;

#include <windows.h>

module Engine.Base.Memory.Hook;
import :ObserverCamera;

import Common.Math.Type;
import Common.Math.System;
import Engine.Base.Hook.System;
import std;

namespace
{
    namespace VecMath = Common::Math::System;

    using Common::Math::Type::Vec3;
    using Engine::Base::Memory::System::GameCameraService;
    using Engine::Base::Memory::Type::ObserverCamera::CameraOverride;

    using Engine::Base::Hook::System::k_DrainTimeout;
    using Engine::Base::Memory::Type::Signature::k_ObserverCameraPrologue;
    using Engine::Base::Memory::Type::Signature::k_ObserverCameraRva;
    using Engine::Base::Memory::Type::Signature::k_ObserverCameraWindow;
    using Engine::Base::Memory::Type::Signature::k_TelemetryIdModifier;
    using Engine::Base::Memory::Type::ObserverCamera::k_ObserverUserStride;
    using Engine::Base::Memory::Type::ObserverCamera::k_ObserverParentFlag;
    using Engine::Base::Memory::Type::ObserverCamera::k_ObserverWorkAxes;
    using Engine::Base::Memory::Type::ObserverCamera::k_ObserverWorkAxesSize;
    using Engine::Base::Memory::Type::ObserverCamera::k_ObserverWorkDistance;
    using Engine::Base::Memory::Type::ObserverCamera::k_ObserverWorkDistanceSize;
    using Engine::Base::Memory::Type::ObserverCamera::k_ObserverWorkFocus;
    using Engine::Base::Memory::Type::ObserverCamera::k_ObserverWorkFocusSize;
    using Engine::Base::Memory::Type::ObserverCamera::k_OverrideMaxAge;
    using Engine::Base::Memory::Type::ObserverCamera::k_OverrideUser;
    using Engine::Base::Memory::Type::ObserverCamera::k_ThreadContextObserverGlobalsOffset;

    // What the hook changes in the observer of the user, kept on the stack to put it back.
    struct WorkBackup
    {
        std::array<std::uint8_t, k_ObserverWorkFocusSize> Focus{};
        std::array<std::uint8_t, k_ObserverWorkDistanceSize> Distance{};
        std::array<std::uint8_t, k_ObserverWorkAxesSize> Axes{};
        std::uint8_t ParentFlag{};
        bool IsSaved{};
    };

    // return: Whether the value can be a pointer to a structure of the engine (not null and aligned to 8 bytes).
    auto IsPlausiblePointer(std::uintptr_t value) -> bool
    {
        return value != 0 && (value & 7) == 0;
    }

    // return: The TLS slot of the thread context found from the code of the game, or nullptr.
    auto ResolveSlot(std::uintptr_t match) -> const std::uint32_t*
    {
        __try
        {
            return Engine::Base::Hook::System::ResolveTlsIndex(match);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return nullptr;
        }
    }

    // return: The observer block of the user in the thread context of the calling thread, or nullptr when there is none.
    // param outFaulted: Set when reading the engine memory faulted.
    // note: Game thread only. Guarded: a wrong slot or a stale context must not crash the game.
    auto FindObserver(const std::uint32_t* tlsIndex, std::uint32_t user, bool& outFaulted) -> std::uint8_t*
    {
        __try
        {
            const std::uintptr_t context{ Engine::Base::Hook::System::ReadThreadContext(*tlsIndex) };
            if (!IsPlausiblePointer(context)) return nullptr;

            const std::uintptr_t globals{ *reinterpret_cast<const std::uintptr_t*>(context + k_ThreadContextObserverGlobalsOffset) };
            if (!IsPlausiblePointer(globals)) return nullptr;

            return reinterpret_cast<std::uint8_t*>(globals + (user * k_ObserverUserStride));
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            outFaulted = true;
            return nullptr;
        }
    }

    // return: The pose with forward and up made exactly orthonormal (Gram-Schmidt), so the engine's own check never replaces them.
    // note: Called with a pose that IsValidOverride accepted, so no axis is near zero or parallel to the other.
    auto Orthonormalize(const CameraOverride& pose) -> CameraOverride
    {
        const Vec3 forward{ VecMath::Normalize(pose.Forward) };
        const Vec3 up{ VecMath::Normalize(VecMath::Subtract(pose.Up, VecMath::Scale(forward, VecMath::Dot(pose.Up, forward)))) };

        return CameraOverride{ .Eye = pose.Eye, .Forward = forward, .Up = up };
    }

    // Saves what is going to be written, then writes the pose: focus = eye, offset 0, distance 0, forward, up, and the parent flag off.
    // return: False when the memory could not be written. The backup says whether something was saved.
    auto ApplyPose(std::uint8_t* observer, const CameraOverride& pose, WorkBackup& backup) -> bool
    {
        __try
        {
            std::memcpy(backup.Focus.data(), observer + k_ObserverWorkFocus, k_ObserverWorkFocusSize);
            std::memcpy(backup.Distance.data(), observer + k_ObserverWorkDistance, k_ObserverWorkDistanceSize);
            std::memcpy(backup.Axes.data(), observer + k_ObserverWorkAxes, k_ObserverWorkAxesSize);
            backup.ParentFlag = observer[k_ObserverParentFlag];
            backup.IsSaved = true;

            const std::array<float, 6> focus{ pose.Eye.X, pose.Eye.Y, pose.Eye.Z, 0.0f, 0.0f, 0.0f };
            const float distance{ 0.0f };
            const std::array<float, 6> axes{ pose.Forward.X, pose.Forward.Y, pose.Forward.Z, pose.Up.X, pose.Up.Y, pose.Up.Z };

            std::memcpy(observer + k_ObserverWorkFocus, focus.data(), sizeof(focus));
            std::memcpy(observer + k_ObserverWorkDistance, &distance, sizeof(distance));
            std::memcpy(observer + k_ObserverWorkAxes, axes.data(), sizeof(axes));

            observer[k_ObserverParentFlag] = 0;
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }

    // return: False when the memory could not be written.
    auto RestorePose(std::uint8_t* observer, const WorkBackup& backup) -> bool
    {
        __try
        {
            std::memcpy(observer + k_ObserverWorkFocus, backup.Focus.data(), k_ObserverWorkFocusSize);
            std::memcpy(observer + k_ObserverWorkDistance, backup.Distance.data(), k_ObserverWorkDistanceSize);
            std::memcpy(observer + k_ObserverWorkAxes, backup.Axes.data(), k_ObserverWorkAxesSize);
            observer[k_ObserverParentFlag] = backup.ParentFlag;
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }
}

namespace Engine::Base::Memory::Hook
{
    // note: The in flight scope spans the call of the original, so Uninstall waits for the threads inside the trampoline too. Nothing here
    // allocates, logs or waits: it runs on the game thread once per frame. The original is always called when there is one: the statics are only
    // cleared after the hook is removed and drained.
    auto __fastcall ObserverCameraDetour::HookedObserverCamera(std::uint32_t user, void* out) -> void
    {
        Engine::Base::Hook::System::InFlightScope scope{ s_InFlight };

        ObserverCameraDetour* self{ s_Instance.load() };
        const ObserverCameraFunction original{ s_OriginalFunction };
        if (original == nullptr) return;

        if (self == nullptr || user != k_OverrideUser)
        {
            original(user, out);
            return;
        }

        // A pose that is not valid, or older than k_OverrideMaxAge, is not written this frame.
        const std::optional<CameraOverride> pose{ self->m_OverrideStore.Read(k_OverrideMaxAge) };
        const std::uint32_t* const tlsIndex{ self->m_EngineTlsIndex.load() };
        std::uint8_t* observer{ nullptr };
        WorkBackup backup{};

        if (pose && GameCameraService::IsValidOverride(*pose) && tlsIndex != nullptr)
        {
            bool isFaulted{ false };
            observer = FindObserver(tlsIndex, user, isFaulted);

            if (observer != nullptr)
            {
                const CameraOverride exact{ Orthonormalize(*pose) };

                if (!ApplyPose(observer, exact, backup))
                {
                    isFaulted = true;
                    if (backup.IsSaved && !RestorePose(observer, backup)) self->m_OverrideStore.CountFault();
                    observer = nullptr;
                }
            }

            if (isFaulted) self->m_OverrideStore.CountFault();
        }

        original(user, out);

        if (observer != nullptr && !RestorePose(observer, backup)) self->m_OverrideStore.CountFault();
    }

    auto ObserverCameraDetour::Install() -> bool
    {
        const std::lock_guard<std::mutex> lock{ m_Mutex };

        if (m_IsInstalled.load()) return true;

        const std::uint32_t* tlsIndex{ ResolveSlot(m_AOBService.FindPattern(k_TelemetryIdModifier)) };
        if (tlsIndex == nullptr)
        {
            m_Log.Message("[ObserverCameraDetour] ERROR: The TLS slot of the thread context was not found, the observer camera is not hooked.");
            return false;
        }

        // The window is 0: the prologue must begin exactly at the RVA.
        void* functionAddress{ reinterpret_cast<void*>(m_AOBService.FindFunctionAtRva(
            k_ObserverCameraPrologue, k_ObserverCameraRva, k_ObserverCameraWindow)) };

        if (functionAddress == nullptr)
        {
            m_Log.Message("[ObserverCameraDetour] ERROR: The observer camera was not found at rva {:X}, its hook is not installed.",
                k_ObserverCameraRva);
            return false;
        }

        m_EngineTlsIndex.store(tlsIndex);
        s_Instance.store(this);

        if (!Engine::Base::Hook::System::InstallDetour(functionAddress, reinterpret_cast<void*>(&HookedObserverCamera),
            reinterpret_cast<void**>(&s_OriginalFunction), "[ObserverCameraDetour]", m_Log))
        {
            s_Instance.store(nullptr);
            s_OriginalFunction = nullptr;
            m_EngineTlsIndex.store(nullptr);
            return false;
        }

        m_FunctionAddress.store(functionAddress);
        m_IsInstalled.store(true);
        m_Log.Message("[ObserverCameraDetour] INFO: The observer camera is hooked at rva {:X}.", k_ObserverCameraRva);
        return true;
    }

    auto ObserverCameraDetour::Uninstall() -> void
    {
        const std::lock_guard<std::mutex> lock{ m_Mutex };

        if (!m_IsInstalled.load()) return;

        void* functionAddress{ m_FunctionAddress.exchange(nullptr) };

        if (functionAddress != nullptr)
        {
            Engine::Base::Hook::System::DisableDetour(functionAddress);

            if (!Engine::Base::Hook::System::WaitForDrain(s_InFlight, k_DrainTimeout))
            {
                m_Log.Message("[ObserverCameraDetour] WARNING: A call was still running when the hook was removed.");
            }

            Engine::Base::Hook::System::RemoveDetour(functionAddress, "[ObserverCameraDetour]", m_Log);
        }

        // Only now: until the hook is removed a thread can still be inside the detour.
        s_OriginalFunction = nullptr;
        s_Instance.store(nullptr);
        m_EngineTlsIndex.store(nullptr);
        m_IsInstalled.store(false);
    }
}