module Perception.Details.System;
import :VitalityDetails;

import Common.Semantic.Type;
import Perception.Classifier.Type;

namespace
{
    using Common::Semantic::Type::Capability;
    using Common::Semantic::Type::HasCapability;
    using Perception::Classifier::Type::Classified;
    using Perception::Details::Type::VitalityDetails;
    using Perception::Details::Type::k_LifeSection;
}

namespace Perception::Details::System
{
    auto VitalityDetailsService::Update() -> void
    {
        auto classifiedsPtr = m_ClassifierStore.Acquire();
        auto objectTablePtr = m_ObjectStore.Acquire();
        if (!classifiedsPtr || !objectTablePtr) return;

        VitalityDetails vitalities{};

        for (const Classified& classified : *classifiedsPtr)
        {
            if (!HasCapability(classified.Capabilities, Capability::HasVitality)) continue;

            const auto objectIt{ objectTablePtr->find(classified.Handle) };
            if (objectIt == objectTablePtr->end()) continue;

            if (std::optional<VitalityDetail> detail{ this->MakeVitality(objectIt->second) })
            {
                vitalities.emplace(classified.Handle, std::move(*detail));
            }
        }

        m_VitalityDetailsStore.Publish(std::move(vitalities));
    }

    auto VitalityDetailsService::MakeVitality(const AliveObject& object) const -> std::optional<VitalityDetail>
    {
        std::shared_ptr<const Vitality> layout{ m_VitalityStore.GetShared(object.TagName) };
        if (!layout || layout->Sections.empty()) return std::nullopt;

        const auto* damage{ m_DamageSectionsStore.Get(object.Handle) };
        if (damage == nullptr || damage->Sections.size() <= k_LifeSection) return std::nullopt;

        VitalityDetail detail{};
        detail.Handle = object.Handle;
        detail.Sections.assign(layout->Sections.size(), 0.0f);

        for (std::size_t i{ 0 }; i < layout->Sections.size(); ++i)
        {
            const int sectionIndex{ layout->Sections[i].SectionIndex };
            if (sectionIndex < 0 || static_cast<std::size_t>(sectionIndex) >= damage->Sections.size()) continue;

            detail.Sections[i] = damage->Sections[sectionIndex].Vitality;
        }

        detail.Layout = std::move(layout);

        return detail;
    }

    auto VitalityDetailsService::Cleanup() -> void
    {
        m_VitalityDetailsStore.Cleanup();

        m_Log.Message("[VitalityDetailsService] INFO: Cleanup completed.");
    }
}