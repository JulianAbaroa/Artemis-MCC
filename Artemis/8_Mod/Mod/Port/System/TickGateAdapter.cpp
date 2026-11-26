module Mod.Port.System;
import :TickGate;

import std;

namespace Mod::Port::System
{
    auto TickGateAdapter::IsTickHeld() -> bool
    {
        return m_BridgeService.HoldTick();
    }
}