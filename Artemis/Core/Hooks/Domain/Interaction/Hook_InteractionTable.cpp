#include "pch.h"

// Header.
#include "Hook_InteractionTable.h"

// --- Systems ---
#include "Core/Systems/Core_System.h"
#include "Core/Systems/Infrastructure/Core_System_Infrastructure.h"
#include "Core/Systems/Infrastructure/Engine/Memory/System_AOBScanner.h"

uintptr_t Hook_InteractionTable::GetInteractionTable()
{
	__try
	{
		uintptr_t tlsArray = (uintptr_t)__readgsqword(0x58);
		uintptr_t match = g_pSystem->Infrastructure->AOBScanner->FindPattern(Signatures::TelemetryIdModifier);

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
					uintptr_t hudGlobalsPtr = *(uintptr_t*)(threadContext + 0xD8);

					if (hudGlobalsPtr)
					{
						return hudGlobalsPtr + 0x6D0;
					}
				}
			}
		}

		return 0;
	}
	__except (EXCEPTION_EXECUTE_HANDLER) { return 0; }
}