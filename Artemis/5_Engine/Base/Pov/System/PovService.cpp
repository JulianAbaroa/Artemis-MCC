module;

#include <windows.h>

module Engine.Base.Pov.System;

import Common.Engine.Type;
import Engine.Base.Hook.System;
import Engine.Base.Memory.Type;
import Blam.Layout.Object.Type;
import Blam.Layout.Player.Type;
import std;

namespace
{
    using Common::Engine::Type::k_HandleIndexMask;
    using Common::Engine::Type::k_MaxLocalUsers;
    using Common::Engine::Type::k_MaxPlayers;
    using Common::Engine::Type::k_NoHandle;
    using Common::Engine::Type::k_SaltShift;

    using Blam::Layout::Object::Type::Constant::k_TelemetryDataPlayerTableOffset;
    using Blam::Layout::Object::Type::Constant::k_ThreadContextMappingOffset;
    using Blam::Layout::Object::Type::Constant::k_ThreadContextTelemetryDataOffset;
    using Blam::Layout::Player::Type::Mapping::k_ControllerPlayer;
    using Blam::Layout::Player::Type::Mapping::k_EntryStride;
    using Blam::Layout::Player::Type::Mapping::k_InputUserPlayer;
    using Blam::Layout::Player::Type::Mapping::k_OutputUserPlayer;
    using Blam::Layout::Player::Type::Offset::k_Handle;
    using Blam::Layout::Player::Type::Offset::k_Loadout;
    using Blam::Layout::Player::Type::Offset::k_LoadoutAcknowledged;
    using Blam::Layout::Player::Type::Size::k_Base;

    using Engine::Base::Memory::Type::Signature::k_AssignOutputUserPrologue;
    using Engine::Base::Memory::Type::Signature::k_AssignOutputUserRva;
    using Engine::Base::Memory::Type::Signature::k_AssignOutputUserWindow;
    using Engine::Base::Pov::System::ShouldAckLoadout;
    using Engine::Base::Pov::Type::PovAssignFunction;
    using Engine::Base::Pov::Type::PovRequest;
    using Engine::Base::Pov::Type::PovStatus;
    using Engine::Base::Pov::Type::k_PovReleasePoll;
    using Engine::Base::Pov::Type::k_PovRequestMaxAge;
    using Engine::Base::Pov::Type::k_PovUser;

    // The handles of the player mapping that the POV needs.
    struct MappingHandles
    {
        std::uint32_t EngineHandle{ k_NoHandle };
        std::uint32_t LocalHandle{ k_NoHandle };
    };

    // return: Whether the mapping was read. Guarded: the thread context can be stale.
    auto ReadMapping(std::uintptr_t threadContext, MappingHandles& out) -> bool
    {
        __try
        {
            const std::uintptr_t mapping{ *reinterpret_cast<const std::uintptr_t*>(threadContext + k_ThreadContextMappingOffset) };
            if (mapping == 0) return false;

            out.EngineHandle = *reinterpret_cast<const std::uint32_t*>(mapping + k_OutputUserPlayer + (k_PovUser * k_EntryStride));
            out.LocalHandle = *reinterpret_cast<const std::uint32_t*>(mapping + k_InputUserPlayer + (k_PovUser * k_EntryStride));
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }

    // return: Whether the handle is a player of the table with the same salt as the one in the table. Guarded.
    // note: A handle that outlived a map load is the case this guards. The Viewer sets the observed player at every scene update (none without players
    // in the tick) and the request ages out in k_PovRequestMaxAge, so an old handle lives a few frames at most, and the salt of the entry must match
    // the handle: a new table does not reuse the pair by design, only by chance, and a wrong player in the view is then one frame long.
    auto IsPlayerHandleValid(std::uintptr_t threadContext, std::uint32_t handle) -> bool
    {
        const std::uint32_t index{ handle & k_HandleIndexMask };
        const std::uint32_t salt{ handle >> k_SaltShift };
        if (handle == k_NoHandle || salt == 0 || index >= k_MaxPlayers) return false;

        __try
        {
            const std::uintptr_t telemetryData{ *reinterpret_cast<const std::uintptr_t*>(threadContext + k_ThreadContextTelemetryDataOffset) };
            if (telemetryData == 0) return false;

            const std::uintptr_t table{ *reinterpret_cast<const std::uintptr_t*>(telemetryData + k_TelemetryDataPlayerTableOffset) };
            if (table == 0) return false;

            return *reinterpret_cast<const std::uint16_t*>(table + (index * k_Base) + k_Handle) == salt;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }

    // Acknowledges the loadout of a player of the table: writes the byte the UI reads (k_LoadoutAcknowledged) equal to the chosen one, when
    // ShouldAckLoadout says so (VW.9a.x, D345). The handle is resolved here, at every call: no pointer is kept between ticks.
    // return: Whether the byte was written. Guarded (access violations only): the only access to engine memory that writes, one byte, the same
    // mark the game makes for the player of a controller.
    auto AckLoadout(std::uintptr_t threadContext, std::uint32_t handle) -> bool
    {
        const std::uint32_t index{ handle & k_HandleIndexMask };
        const std::uint32_t salt{ handle >> k_SaltShift };
        if (handle == k_NoHandle || salt == 0 || index >= k_MaxPlayers) return false;

        __try
        {
            const std::uintptr_t mapping{ *reinterpret_cast<const std::uintptr_t*>(threadContext + k_ThreadContextMappingOffset) };
            const std::uintptr_t telemetryData{ *reinterpret_cast<const std::uintptr_t*>(threadContext + k_ThreadContextTelemetryDataOffset) };
            if (mapping == 0 || telemetryData == 0) return false;

            const std::uintptr_t table{ *reinterpret_cast<const std::uintptr_t*>(telemetryData + k_TelemetryDataPlayerTableOffset) };
            if (table == 0) return false;

            std::array<std::uint32_t, k_MaxLocalUsers> controllerPlayers{};
            for (std::size_t controller{ 0 }; controller < k_MaxLocalUsers; ++controller)
            {
                controllerPlayers[controller] = *reinterpret_cast<const std::uint32_t*>(mapping + k_ControllerPlayer + (controller * k_EntryStride));
            }

            // The entry must still be this player: the guard of the only write is checked here, with no engine code run in between.
            const std::uintptr_t player{ table + (index * k_Base) };
            if (*reinterpret_cast<const std::uint16_t*>(player + k_Handle) != salt) return false;

            const std::uint8_t chosen{ *reinterpret_cast<const std::uint8_t*>(player + k_Loadout) };
            const std::uint8_t acknowledged{ *reinterpret_cast<const std::uint8_t*>(player + k_LoadoutAcknowledged) };
            if (!ShouldAckLoadout(handle, controllerPlayers, chosen, acknowledged)) return false;

            *reinterpret_cast<std::uint8_t*>(player + k_LoadoutAcknowledged) = chosen;
            return true;
        }
        __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
        {
            return false;
        }
    }

    // return: False if the engine function faulted.
    auto CallAssign(PovAssignFunction assign, std::uint32_t handle) -> bool
    {
        __try
        {
            assign(k_PovUser, handle);
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }
}

namespace Engine::Base::Pov::System
{
    auto DecidePovCall(std::optional<std::uint32_t> requested, std::uint32_t engineCurrent, std::uint32_t local, bool isRequestedValid, bool ownsView)
        -> std::optional<std::uint32_t>
    {
        if (requested && isRequestedValid)
        {
            if (*requested == engineCurrent) return std::nullopt;
            return *requested;
        }

        if (!ownsView || local == k_NoHandle || local == engineCurrent) return std::nullopt;
        return local;
    }

    auto NextAssigned(std::uint32_t previous, std::optional<std::uint32_t> assignedNow, std::uint32_t engineCurrent, std::uint32_t local) -> std::uint32_t
    {
        const std::uint32_t viewAfter{ assignedNow ? *assignedNow : engineCurrent };

        if (local != k_NoHandle && viewAfter == local) return k_NoHandle;
        if (assignedNow) return *assignedNow;
        return previous;
    }

    auto ShouldAckLoadout(std::uint32_t playerHandle, const std::array<std::uint32_t, k_MaxLocalUsers>& controllerPlayers, std::uint8_t chosen,
        std::uint8_t acknowledged) -> bool
    {
        if (playerHandle == k_NoHandle || chosen == acknowledged) return false;
        return std::ranges::find(controllerPlayers, playerHandle) == controllerPlayers.end();
    }

    auto IsPovOnOtherPlayer(const PovStatus& status) -> bool
    {
        return status.RequestedHandle != k_NoHandle && status.EngineHandle == status.RequestedHandle && status.EngineHandle != status.LocalHandle;
    }

    auto PovService::Install() -> bool
    {
        if (m_Assign.load() != nullptr) return true;

        // The window is 0: the prologue must begin exactly at the RVA.
        const std::uintptr_t address{ m_AOBService.FindFunctionAtRva(k_AssignOutputUserPrologue, k_AssignOutputUserRva, k_AssignOutputUserWindow) };

        if (address == 0)
        {
            m_Log.Message("[PovService] ERROR: The view assignment was not found at rva {:X}, the POV is not available.", k_AssignOutputUserRva);
            m_StatusStore.SetInstalled(false);
            return false;
        }

        // A new engine starts with no view of its own: nothing of a previous one (handles, ownership, faults) is kept.
        m_StatusStore.Reset();
        m_IsReleased.store(false);
        m_Assign.store(reinterpret_cast<PovAssignFunction>(address));
        m_StatusStore.SetInstalled(true);
        m_Log.Message("[PovService] INFO: The view assignment was found at rva {:X}.", k_AssignOutputUserRva);
        return true;
    }

    auto PovService::Uninstall() -> void
    {
        m_Assign.store(nullptr);
        m_GameThread.store(std::thread::id{});
        m_GameThreadContext.store(0);
        m_StatusStore.Reset();
    }

    auto PovService::Disable() -> void
    {
        m_StatusStore.CountFault();

        // The first fault is the one that logs: the exchange tells it.
        if (m_Assign.exchange(nullptr) != nullptr)
        {
            m_Log.Message("[PovService] ERROR: The view assignment faulted, the POV is off until the next engine.");
        }

        m_StatusStore.SetInstalled(false);
    }

    auto PovService::Update(std::uintptr_t threadContext) -> void
    {
        const PovAssignFunction assign{ m_Assign.load() };
        if (assign == nullptr || threadContext == 0) return;

        // Update runs on the game thread only: Release finds it through these.
        m_GameThread.store(std::this_thread::get_id());
        m_GameThreadContext.store(threadContext);

        MappingHandles mapping{};
        if (!ReadMapping(threadContext, mapping)) return;

        const std::optional<PovRequest> request{ m_IsReleased.load() ? std::nullopt : m_RequestStore.Read(k_PovRequestMaxAge) };
        std::optional<std::uint32_t> requested{};
        if (request && request->PlayerHandle != k_NoHandle) requested = request->PlayerHandle;

        const bool isRequestedValid{ requested && IsPlayerHandleValid(threadContext, *requested) };

        // A request that begins starts the counts over, so the telemetry shows what this one did.
        const PovStatus previous{ m_StatusStore.Read() };
        if (requested && previous.RequestedHandle == k_NoHandle) m_StatusStore.ResetCounts();

        // The view is this service's only while it holds a player it assigned: with none, a view that differs from the local player is somebody
        // else's (the theater, a join in progress) and stays.
        const bool ownsView{ previous.AssignedHandle != k_NoHandle };

        const std::optional<std::uint32_t> target{ DecidePovCall(requested, mapping.EngineHandle, mapping.LocalHandle, isRequestedValid, ownsView) };
        std::optional<std::uint32_t> assignedNow{};

        if (target)
        {
            const bool isRequestedTarget{ isRequestedValid && *target == *requested };

            // It is a reassert when the player was already set by this service and the engine took it away.
            const bool isReassert{ isRequestedTarget && previous.AssignedHandle == *target };

            if (CallAssign(assign, *target))
            {
                m_StatusStore.CountCall(isReassert);
                assignedNow = *target;
            }
            else
            {
                this->Disable();
            }
        }

        const std::uint32_t assigned{ NextAssigned(previous.AssignedHandle, assignedNow, mapping.EngineHandle, mapping.LocalHandle) };
        m_StatusStore.SetHandles(requested.value_or(k_NoHandle), mapping.EngineHandle, mapping.LocalHandle, assigned);

        // The engine view already shows a requested player that is not the local one (not on the tick that assigned it): the UI never
        // acknowledges his loadout (D345), so this does.
        if (isRequestedValid && mapping.EngineHandle == *requested && mapping.EngineHandle != mapping.LocalHandle && AckLoadout(threadContext, mapping.EngineHandle))
        {
            m_StatusStore.CountLoadoutAck();
        }
    }

    auto PovService::RestoreView(std::uintptr_t threadContext) -> void
    {
        const PovAssignFunction assign{ m_Assign.load() };
        if (assign == nullptr) return;

        MappingHandles mapping{};
        if (!ReadMapping(threadContext, mapping)) return;

        if (mapping.LocalHandle != k_NoHandle && mapping.LocalHandle != mapping.EngineHandle)
        {
            if (!CallAssign(assign, mapping.LocalHandle))
            {
                this->Disable();
                return;
            }

            m_Log.Message("[PovService] INFO: The view went back to the local player.");
        }

        m_StatusStore.SetHandles(k_NoHandle, mapping.LocalHandle, mapping.LocalHandle, k_NoHandle);
    }

    auto PovService::Release(std::chrono::milliseconds timeout) -> void
    {
        if (m_Assign.load() == nullptr) return;

        // First: from now on Update ignores the request, so it cannot take the view again while this checks the ownership.
        m_IsReleased.store(true);

        if (m_StatusStore.Read().AssignedHandle == k_NoHandle) return;

        // On the game thread nobody else can serve the return, so it is done here with the context of the last Update, and it never waits.
        if (std::this_thread::get_id() == m_GameThread.load())
        {
            const std::uintptr_t threadContext{ m_GameThreadContext.load() };
            if (threadContext != 0) this->RestoreView(threadContext);
            return;
        }

        // Another thread: Update does it, in a frame, once it ignores the request.
        const auto deadline{ std::chrono::steady_clock::now() + timeout };
        while (m_Assign.load() != nullptr && m_StatusStore.Read().AssignedHandle != k_NoHandle)
        {
            if (std::chrono::steady_clock::now() >= deadline)
            {
                m_Log.Message("[PovService] WARNING: The game thread did not give the view back to the local player in time.");
                return;
            }

            std::this_thread::sleep_for(k_PovReleasePoll);
        }
    }
}