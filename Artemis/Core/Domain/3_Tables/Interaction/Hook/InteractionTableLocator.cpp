module;

#include <windows.h>

module Tables.Interaction.Hook;

import Platform.Memory.Type;

namespace
{
    namespace Signature = Platform::Memory::Type::Signature;

    // Sanity limit of the telemetry index. Anything above it is not a valid thread slot.
    constexpr std::uint32_t k_MaxTelemetryId{ 1000 };

    // Offset of the HUD globals pointer inside the thread context.
    constexpr std::uintptr_t k_HudGlobalsOffset{ 0xD8 };

    // Offset of the interaction table inside the HUD globals.
    constexpr std::uintptr_t k_InteractionTableOffset{ 0x6D0 };
}

namespace Tables::Interaction::Hook
{
    auto InteractionTableLocator::FindAndStoreTableBase() -> void
    {
        std::uintptr_t tableBase{ m_InteractionStore.GetBase() };

        if (tableBase == 0)
        {
            tableBase = this->GetInteractionTable();
            if (!tableBase)
            {
                m_LogsService.Message("[InteractionTableLocator] ERROR:"
                    " Failed to find the interaction table.");

                return;
            }

            m_LogsService.Message("[InteractionTableLocator] INFO:"
                " Interaction table found at {:016X}.", tableBase);

            m_InteractionStore.SetBase(tableBase);
        }
    }

    auto InteractionTableLocator::GetInteractionTable() -> std::uintptr_t
    {
        __try
        {
            // note: gs:[0x58] is the thread local storage array of the current thread.
            std::uintptr_t tlsArray{ (std::uintptr_t)__readgsqword(0x58) };
            std::uintptr_t match{ m_AOBService.FindPattern(
                Signature::TelemetryIdModifier) };

            if (match)
            {
                // note: The match is a "mov ecx, [rip+rel32]". The rel32 is at +2 and the instruction is 6 bytes long.
                std::int32_t relativeOffset{ *(std::int32_t*)(match + 2) };
                std::uintptr_t telemetryIdAddr{ (match + 6) + relativeOffset };
                std::uint32_t telemetryIdx{ *(std::uint32_t*)(telemetryIdAddr) };

                if (telemetryIdx <= k_MaxTelemetryId)
                {
                    std::uintptr_t threadContext{ *(std::uintptr_t*)(tlsArray +
                        (static_cast<unsigned long long>(telemetryIdx) * 8)) };

                    if (threadContext)
                    {
                        std::uintptr_t hudGlobalsPtr{
                            *(std::uintptr_t*)(threadContext + k_HudGlobalsOffset) };

                        if (hudGlobalsPtr)
                        {
                            return hudGlobalsPtr + k_InteractionTableOffset;
                        }
                    }
                }
            }

            return 0;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) { return 0; }
    }
}