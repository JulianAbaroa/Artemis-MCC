#include "pch.h"

// Header.
#include "Hook_InteractionTable.h"

// Types.
#include "Core/Types/Infrastructure/AOB/Signatures.h"

// --- States ---

#include "Core/States/Domain/Interaction/State_InteractionTable.h"

// --- Systems ---

#include "Core/Systems/Infrastructure/Engine/Memory/System_AOBScanner.h"

#include "Core/Systems/Interface/Debug/System_Debug.h"

void Hook_InteractionTable::FindAndStoreTableBase()
{
	uintptr_t tableBase = m_Deps.State_InteractionTable.GetInteractionTableBase();
	if (tableBase == 0)
	{
		tableBase = this->GetInteractionTable();
		if (!tableBase)
		{
			m_Deps.System_Debug.Log("[InteractionTableSystem] ERROR:"
				" InteractionTableBase invalid.");

			return;
		}

		m_Deps.System_Debug.Log("[InteractionTableSystem] INFO: InteractionTable: 0x%llX", tableBase);
		m_Deps.State_InteractionTable.SetInteractionTableBase(tableBase);
	}
}

uintptr_t Hook_InteractionTable::GetInteractionTable()
{
	__try
	{
		uintptr_t tlsArray = (uintptr_t)__readgsqword(0x58);
		uintptr_t match = m_Deps.System_AOBScanner.
			FindPattern(Signatures::TelemetryIdModifier);

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