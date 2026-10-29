module Mod.Port.System;
import :ActionSource;

import std;

namespace Mod::Port::System
{
    auto BridgeActionSource::Prepare(std::uint64_t actionTick) -> void
    {
        m_BridgeService.Apply(actionTick);
    }
}