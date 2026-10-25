module;

#include <windows.h>

module Platform.Capture.Hook;
import :InteractionTable;

import Platform.Hook.System;
import Platform.Memory.Type;
import Datum.Object.Type;

namespace
{
    namespace Signature = Platform::Memory::Type::Signature;

    using Datum::Object::Type::Constant::k_ThreadContextControlGlobalsOffset;
    using Datum::Object::Type::Constant::k_ControlGlobalsOutputUserTableOffset;
}

namespace Platform::Capture::Hook
{
    auto InteractionTableLocator::FindAndStoreTableBase() -> void
    {
        std::uintptr_t tableBase{ m_CaptureStore.GetInteractionTableBase() };

        if (tableBase == 0)
        {
            tableBase = this->GetInteractionTable();
            if (!tableBase)
            {
                m_Log.Message("[InteractionTableLocator] ERROR:"
                    " Failed to find the interaction tables.");

                return;
            }

            m_Log.Message("[InteractionTableLocator] INFO:"
                " Interaction tables found at {:016X}.", tableBase);

            m_CaptureStore.SetInteractionTableBase(tableBase);
        }
    }

    auto InteractionTableLocator::GetInteractionTable() -> std::uintptr_t
    {
        __try
        {
            const std::uint32_t* tlsIndex{ Platform::Hook::System::ResolveTlsIndex(
                m_AOBService.FindPattern(Signature::k_TelemetryIdModifier)) };
            if (!tlsIndex) return 0;

            const std::uintptr_t threadContext{ Platform::Hook::System::ReadThreadContext(*tlsIndex) };
            if (!threadContext) return 0;

            const std::uintptr_t controlGlobals{
                *reinterpret_cast<const std::uintptr_t*>(threadContext + k_ThreadContextControlGlobalsOffset) };
            if (!controlGlobals) return 0;

            return controlGlobals + k_ControlGlobalsOutputUserTableOffset;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return 0;
        }
    }
}