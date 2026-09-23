module;

#include <windows.h>

module Tables.Interaction.Hook;

import Platform.Memory.Type;

namespace
{
	namespace Signature = Platform::Memory::Type::Signature;
}

namespace Tables::Interaction::Hook
{
	auto InteractionTableLocator::FindAndStoreTableBase() -> void
	{
		std::uintptr_t tableBase = m_InteractionStore.GetBase();

		if (tableBase == 0)
		{
			tableBase = this->GetInteractionTable();
			if (!tableBase)
			{
				m_LogsService.Message("[InteractionTableLocator] ERROR:"
					" InteractionTableBase invalid.");

				return;
			}

			m_LogsService.Message("[InteractionTableLocator] INFO:"
				" InteractionTable: {:016X}", tableBase);

			m_InteractionStore.SetBase(tableBase);
		}
	}

	auto InteractionTableLocator::GetInteractionTable() -> std::uintptr_t
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
						std::uintptr_t hudGlobalsPtr = 
							*(std::uintptr_t*)(threadContext + 0xD8);

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
}