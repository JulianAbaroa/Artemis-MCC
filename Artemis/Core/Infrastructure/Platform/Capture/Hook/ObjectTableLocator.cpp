module;

#include <windows.h>

module Platform.Capture.Hook;
import :ObjectTable;

import Platform.Hook.System;
import Platform.Memory.Type;
import Datum.Object.Type;

namespace
{
    namespace Signature = Platform::Memory::Type::Signature;

    using Datum::Object::Type::Constant::k_ThreadContextObjectDataOffset;
    using Datum::Object::Type::Constant::k_ObjectDataTableOffset;
}

namespace Platform::Capture::Hook
{
    auto ObjectTableLocator::FindAndStoreTableBase() -> void
    {
        if (m_CaptureStore.GetObjectTableBase() != 0) return;

        const std::uintptr_t threadContext{ this->GetThreadContext() };
        const std::uintptr_t tableBase{ this->GetObjectTable(threadContext) };
        if (!tableBase)
        {
            m_Log.Message("[ObjectTableLocator] ERROR:"
                " ObjectTableBase invalid.");
            return;
        }

        m_Log.Message("[ObjectTableLocator] INFO: ObjectTable:"
            " {:016X}", tableBase);

        m_CaptureStore.SetThreadContext(threadContext);
        m_CaptureStore.SetObjectTableBase(tableBase);
    }

    auto ObjectTableLocator::GetThreadContext() -> std::uintptr_t
    {
        __try
        {
            const std::uint32_t* tlsIndex{ Platform::Hook::System::ResolveTlsIndex(
                m_AOBService.FindPattern(Signature::k_TelemetryIdModifier)) };
            if (!tlsIndex) return 0;

            return Platform::Hook::System::ReadThreadContext(*tlsIndex);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return 0;
        }
    }

    // Returns the pointer to the Global Object Table (Indirection Table).
    // This table acts as a lookup array where each 24-byte (0x18) entry maps a
    // networked 'Handle' to a physical memory address.
    // Resolution logic: The lower 16 bits of a Handle (0xFFFF) serve as the index
    // into this table, while the upper 16 bits function as a 'Salt' for versioning.
    // Each entry contains a 64-bit pointer to the actual entity data (Bipeds, Weapons, etc.).
    // Capacity: Probably 1024 slots, supporting the 650 Forge-item limit
    // plus dynamic entities like projectiles and player assets.
    auto ObjectTableLocator::GetObjectTable(std::uintptr_t threadContext) -> std::uintptr_t
    {
        if (!threadContext) return 0;

        __try
        {
            std::uintptr_t telemetryData =
                *(std::uintptr_t*)(threadContext + k_ThreadContextObjectDataOffset);

            if (telemetryData)
            {
                return *(std::uintptr_t*)(telemetryData + k_ObjectDataTableOffset);
            }

            return 0;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return 0;
        }
    }
}