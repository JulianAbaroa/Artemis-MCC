module;

#include <windows.h>

module Engine.Capture.Hook;
import :InteractionTable;

import Engine.Base.Hook.System;
import Blam.Layout.Object.Type;

namespace
{
    using Blam::Layout::Object::Type::Constant::k_ThreadContextControlGlobalsOffset;
    using Blam::Layout::Object::Type::Constant::k_ControlGlobalsOutputUserTableOffset;
}

namespace Engine::Capture::Hook
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
        const std::uintptr_t threadContext{ Engine::Base::Hook::System::FindThreadContext(m_AOBService) };
        if (!threadContext) return 0;

        __try
        {
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