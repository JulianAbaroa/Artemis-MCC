module Platform.Port.System;
import :ActionSource;

import std;

namespace Platform::Port::System
{
    auto BridgeActionSource::Prepare(std::uint64_t actionTick) -> void
    {
        m_BridgeService.Apply(actionTick);
    }
}