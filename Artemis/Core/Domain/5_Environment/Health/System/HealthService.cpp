module Environment.Health.System;

namespace
{
    using AliveObject = Tables::Object::Type::Alive::Object;
    using ObjectTable = std::unordered_map<std::uint32_t, AliveObject>;
    using VitalitySection = Resolved::Vitality::Type::Vitality::Section;
    using Classified = Relations::Classifier::Type::Classified;
    using Classifieds = std::vector<Classified>;
    using Role = Relations::Classifier::Type::Role;
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
            if (!this->HasHealth(classified)) continue;

            auto objectIt = objectTable.find(classified.Handle);
            if (objectIt == objectTable.end()) continue;

            const AliveObject& object = objectIt->second;
            if (object.Address == 0) continue;

            const DamageSectionTable* damage = m_DamageSectionsStore.Get(object.Handle);
            if (!damage || damage->Sections.empty()) continue;

            const ResolvedVitality* layout = m_VitalityStore.
                GetResolvedVitality(object.TagName);
            if (!layout) continue;

            Health health = this->MakeHealth(object, *layout, *damage);
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

    auto HealthService::HasHealth(const Classified& classified) -> bool
    {
        switch (classified.Role)
        {
        case Role::Biped:
        case Role::Vehicle:
        case Role::VehiclePart:
        case Role::PortableShield:
        case Role::Explosive:
        case Role::Pallet:
            return true;
        default:
            return false;
        }
    }

    auto HealthService::Cleanup() -> void
    {
        m_VitalityStore.Cleanup();

        m_LogsService.Message("[HealthService] INFO: Cleanup completed.");
    }
}