#include "pch.h"
#include "Core/Systems/CoreSystem.h"
#include "Core/Systems/Infrastructure/CoreInfrastructureSystem.h"
#include "Core/Systems/Infrastructure/Engine/ScannerSystem.h"
#include "Core/Hooks/Memory/ObjectTableHook.h"

// Returns the pointer to the Global Object Table (Indirection Table).
// This table acts as a lookup array where each 24-byte (0x18) entry maps a 
// networked 'Handle' to a physical memory address.
// Resolution logic: The lower 16 bits of a Handle (0xFFFF) serve as the index
// into this table, while the upper 16 bits function as a 'Salt' for versioning.
// Each entry contains a 64-bit pointer to the actual entity data (Bipeds, Weapons, etc.).
// Capacity: Probably 1024 slots, supporting the 650 Forge-item limit
// plus dynamic entities like projectiles and player assets.
uintptr_t ObjectTableHook::GetObjectTable()
{
    __try
    {
        uintptr_t tlsArray = (uintptr_t)__readgsqword(0x58);

        uintptr_t match = g_pSystem->Infrastructure->Scanner->FindPattern(Signatures::TelemetryIdModifier);

        if (match)
        {
            int32_t relativeOffset = *(int32_t*)(match + 2);
            uintptr_t telemetryIdAddr = (match + 6) + relativeOffset;
            uint32_t telemetryIdx = *(uint32_t*)(telemetryIdAddr);

            if (telemetryIdx <= 1000) 
            {
                uintptr_t threadContext = *(uintptr_t*)(tlsArray + (static_cast<unsigned long long>(telemetryIdx) * 8));

                if (threadContext) 
                {
                    uintptr_t telemetryData = *(uintptr_t*)(threadContext + 0x10);

                    if (telemetryData) 
                    {
                        uintptr_t objectTable = *(uintptr_t*)(telemetryData + 0x50);
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