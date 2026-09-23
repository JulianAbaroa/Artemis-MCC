module;

#include <windows.h>

module Tables.Object.Hook;
import :ObjectTable;

import Platform.Memory.Type;

namespace
{
    namespace Signature = Platform::Memory::Type::Signature;
}

namespace Tables::Object::Hook
{
    auto ObjectTableLocator::FindAndStoreTableBase() -> void
    {
        std::uintptr_t tableBase = m_ObjectStore.GetBase();
        if (tableBase == 0)
        {
            tableBase = this->GetObjectTable();
            if (!tableBase)
            {
                m_LogsService.Message("[ObjectTableLocator] ERROR:"
                    " ObjectTableBase invalid.");
                return;
            }

            m_LogsService.Message("[ObjectTableLocator] INFO: ObjectTable:"
                " {:016X}", tableBase);

            m_ObjectStore.SetBase(tableBase);
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
    auto ObjectTableLocator::GetObjectTable() -> std::uintptr_t
    {
        __try
        {
            std::uintptr_t tlsArray = (std::uintptr_t)__readgsqword(0x58);

            std::uintptr_t match = m_AOBService.FindPattern(
                Signature::TelemetryIdModifier);

            if (match)
            {
                std::int32_t relativeOffset = *(std::int32_t*)(match + 2);
                std::uintptr_t telemetryIdAddr = (match + 6) + relativeOffset;
                std::uint32_t telemetryIdx = *(std::uint32_t*)(telemetryIdAddr);

                if (telemetryIdx <= 1000)
                {
                    std::uintptr_t threadContext = *(std::uintptr_t*)(tlsArray + 
                        (static_cast<unsigned long long>(telemetryIdx) * 8));

                    if (threadContext)
                    {
                        std::uintptr_t telemetryData = 
                            *(std::uintptr_t*)(threadContext + 0x10);

                        if (telemetryData)
                        {
                            std::uintptr_t objectTable = 
                                *(std::uintptr_t*)(telemetryData + 0x50);

                            return objectTable;
                        }
                    }
                }
            }

            return 0;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return 0;
        }
    }
}