module;

#include <windows.h>

module Tables.Player.Hook;

import Platform.Hook.System;
import Platform.Memory.Type;
import Datum.Object.Type;

namespace
{
    namespace Signature = Platform::Memory::Type::Signature;

    using Datum::Object::Type::Constant::k_ThreadContextTelemetryDataOffset;
    using Datum::Object::Type::Constant::k_TelemetryDataPlayerTableOffset;
    using Datum::Object::Type::Constant::k_ThreadContextMappingOffset;
}

namespace Tables::Player::Hook
{
    auto PlayerTableLocator::FindAndStoreTableBase() -> void
    {
        const bool hasTable{ m_PlayerStore.GetBase() != 0 };
        const bool hasMapping{ m_PlayerStore.GetMappingBase() != 0 };
        if (hasTable && hasMapping) return;

        const std::uintptr_t threadContext{ this->GetThreadContext() };

        if (!hasTable)
        {
            const std::uintptr_t tableBase{ this->GetPlayerTable(threadContext) };
            if (!tableBase)
            {
                m_Log.Message("[PlayerTableLocator] ERROR:"
                    " Failed to find the player table.");
            }
            else
            {
                m_Log.Message("[PlayerTableLocator] INFO:"
                    " Player table found at {:016X}.", tableBase);

                m_PlayerStore.SetBase(tableBase);
            }
        }

        if (!hasMapping)
        {
            const std::uintptr_t mappingBase{ this->GetMapping(threadContext) };
            if (!mappingBase)
            {
                m_Log.Message("[PlayerTableLocator] ERROR:"
                    " Failed to find the player mapping.");
            }
            else
            {
                m_Log.Message("[PlayerTableLocator] INFO:"
                    " Player mapping found at {:016X}.", mappingBase);

                m_PlayerStore.SetMappingBase(mappingBase);
            }
        }
    }

    auto PlayerTableLocator::GetThreadContext() -> std::uintptr_t
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

    auto PlayerTableLocator::GetPlayerTable(std::uintptr_t threadContext) -> std::uintptr_t
    {
        if (!threadContext) return 0;

        __try
        {
            std::uintptr_t telemetryData{ *(std::uintptr_t*)(threadContext + k_ThreadContextTelemetryDataOffset) };
            if (!telemetryData) return 0;

            return *(std::uintptr_t*)(telemetryData + k_TelemetryDataPlayerTableOffset);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return 0;
        }
    }

    auto PlayerTableLocator::GetMapping(std::uintptr_t threadContext) -> std::uintptr_t
    {
        if (!threadContext) return 0;

        __try
        {
            return *(std::uintptr_t*)(threadContext + k_ThreadContextMappingOffset);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return 0;
        }
    }
}