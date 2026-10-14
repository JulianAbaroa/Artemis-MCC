module Environment.Health.System;

import Common.Semantic.Type;

namespace
{
    using AliveObject = Tables::Object::Type::Alive::Object;
    using ObjectTable = std::unordered_map<std::uint32_t, AliveObject>;
    using VitalitySection = Resolved::Vitality::Type::Vitality::Section;
    using Classified = Relations::Classifier::Type::Classified;
    using Classifieds = std::vector<Classified>;
    using Common::Semantic::Type::Capability;
    using Common::Semantic::Type::HasCapability;
}

namespace Environment::Health::System
{
    auto HealthService::Update() -> void
    {
        auto classifiedsPtr = m_ClassifierStore.Acquire();
        auto objectTablePtr = m_ObjectStore.Acquire();
        if (!classifiedsPtr || !objectTablePtr) return;

        const Classifieds& classifieds = *classifiedsPtr;
        const ObjectTable& objectTable = *objectTablePtr;

        std::unordered_map<std::uint32_t, Health> result;
        result.reserve(classifieds.size());

        for (const Classified& classified : classifieds)
        {
            if (!HasCapability(classified.Capabilities, Capability::HasVitality)) continue;

            auto objectIt = objectTable.find(classified.Handle);
            if (objectIt == objectTable.end()) continue;

            const AliveObject& object = objectIt->second;
            if (object.Address == 0) continue;

            const DamageSectionTable* damage = m_DamageSectionsStore.Get(object.Handle);
            if (!damage || damage->Sections.empty()) continue;

            const auto layout = m_VitalityStore.GetShared(object.TagName);
            if (!layout) continue;

            Health health = this->MakeHealth(object, *layout, *damage);
            health.Layout = layout;
            result.emplace(object.Handle, std::move(health));
        }

        m_HealthStore.Publish(std::move(result));
    }

    auto HealthService::MakeHealth(const AliveObject& object,
        const ResolvedVitality& layout, const DamageSectionTable& damage) const -> Health
    {
        Health health;
        health.Handle = object.Handle;

        const auto& mem = damage.Sections;

        health.SectionVitalities.assign(layout.Sections.size(), 0.0f);

        for (size_t i = 0; i < layout.Sections.size(); ++i)
        {
            const auto& L = layout.Sections[i];
            if (L.SectionIndex >= 0 &&
                static_cast<size_t>(L.SectionIndex) < mem.size())
            {
                health.SectionVitalities[i] = mem[L.SectionIndex].Vitality;
            }
        }

        if (layout.CriticalSection >= 0 &&
            static_cast<size_t>(layout.CriticalSection) < health.SectionVitalities.size())
        {
            health.IsDead = health.SectionVitalities[layout.CriticalSection] <= 0.0f;
        }

        return health;
    }

    auto HealthService::Cleanup() -> void
    {
        m_VitalityStore.Cleanup();

        m_LogsService.Message("[HealthService] INFO: Cleanup completed.");
    }
}