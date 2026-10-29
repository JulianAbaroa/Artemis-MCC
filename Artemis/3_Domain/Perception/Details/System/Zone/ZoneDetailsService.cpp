module Perception.Details.System;
import :ZoneDetails;

import Common.Semantic.Type;
import Perception.Classifier.Type;

namespace
{
    using Common::Semantic::Type::Capability;
    using Common::Semantic::Type::HasCapability;
    using Perception::Classifier::Type::Classified;
    using Perception::Details::Type::ZoneDetail;
    using Perception::Details::Type::ZoneDetails;
}

namespace Perception::Details::System
{
    auto ZoneDetailsService::Update() -> void
    {
        auto classifiedsPtr = m_ClassifierStore.Acquire();
        auto objectTablePtr = m_ObjectStore.Acquire();
        if (!classifiedsPtr || !objectTablePtr) return;

        ZoneDetails zones{};

        for (const Classified& classified : *classifiedsPtr)
        {
            if (!HasCapability(classified.Capabilities, Capability::HasZone)) continue;

            const auto objectIt{ objectTablePtr->find(classified.Handle) };
            if (objectIt == objectTablePtr->end()) continue;

            // note: HasZone comes from this block, so the check only guards.
            const auto& multiplayer{ objectIt->second.Multiplayer };
            if (!multiplayer) continue;

            zones.emplace(classified.Handle, ZoneDetail{ .Handle = classified.Handle, .Shape = multiplayer->Shape });
        }

        m_ZoneDetailsStore.Publish(std::move(zones));
    }

    auto ZoneDetailsService::Cleanup() -> void
    {
        m_ZoneDetailsStore.Cleanup();

        m_Log.Message("[ZoneDetailsService] INFO: Cleanup completed.");
    }
}