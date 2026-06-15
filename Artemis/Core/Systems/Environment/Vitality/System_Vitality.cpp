#include "pch.h"

#include "System_Vitality.h"

#include "Core/States/Structure/Classifier/State_Classifier.h"
#include "Core/States/Sources/Tables/Object/State_ObjectTable.h"
#include "Core/States/Sources/Tables/Object/DamageSection/State_DamageSections.h"
#include "Core/States/Sources/Static/Vitality/State_VitalityBuilder.h"
#include "Core/States/Environment/Vitality/State_Vitality.h"

#include "Core/Systems/Other/Logs/System_Logs.h"

void System_Vitality::Update()
{
    auto classifiedsPtr = m_Deps.State_Classifier.Acquire();
    auto objectTablePtr = m_Deps.State_ObjectTable.Acquire();
    if (!classifiedsPtr || !objectTablePtr) return;

    const Classifieds& classifieds = *classifiedsPtr;
    const ObjectTable& objectTable = *objectTablePtr;

    std::unordered_map<uint32_t, ObjectVitality> result;
    result.reserve(classifieds.size());

    for (const Classified& classified : classifieds)
    {
        if (!this->HasVitality(classified)) continue;

        auto objectIt = objectTable.find(classified.Handle);
        if (objectIt == objectTable.end()) continue;

        const LiveObject& object = objectIt->second;
        if (object.Address == 0) continue;

        const DamageSectionTable* damage = m_Deps.State_DamageSections.Get(object.Handle);
        if (!damage || damage->Sections.empty()) continue;

        const VitalityLayout* layout =
            m_Deps.State_VitalityBuilder.GetLayout(object.TagName);
        if (!layout) continue;

        ObjectVitality vitality = this->MakeVitality(object, *layout, *damage);
        result.emplace(object.Handle, std::move(vitality));
    }

    m_Deps.State_Vitality.Publish(std::move(result));
}

ObjectVitality System_Vitality::MakeVitality(
    const LiveObject& object, const VitalityLayout& layout,
    const DamageSectionTable& damage) const
{
    ObjectVitality vitality;
    vitality.Handle = object.Handle;
    vitality.CriticalSection = layout.CriticalSection;
    vitality.ShieldSection = layout.ShieldSection;

    const auto& mem = damage.Sections;

    vitality.Sections.resize(layout.Sections.size());

    for (size_t i = 0; i < layout.Sections.size(); ++i)
    {
        const VitalitySectionLayout& L = layout.Sections[i];
        VitalitySection& s = vitality.Sections[i];

        s.NameId = L.NameId;
        s.CollRegion = L.CollRegion;
        s.IsCritical = L.IsCritical;
        s.IsHeadshot = L.IsHeadshot;
        s.Kind = L.Kind;

        if (L.SectionIndex >= 0 &&
            static_cast<size_t>(L.SectionIndex) < mem.size())
        {
            s.Vitality = mem[L.SectionIndex].Vitality;
        }
    }

    if (vitality.CriticalSection >= 0 &&
        static_cast<size_t>(vitality.CriticalSection) < vitality.Sections.size())
    {
        vitality.IsDead = vitality.Sections[vitality.CriticalSection].Vitality <= 0.0f;
    }

    return vitality;
}

bool System_Vitality::HasVitality(const Classified& classified)
{
    switch (classified.Role)
    {
    case ObjectRole::Biped:
    case ObjectRole::Vehicle:
    case ObjectRole::VehiclePart:
    case ObjectRole::PortableShield:
    case ObjectRole::Explosive:
    case ObjectRole::Pallet:
        return true;
    default:
        return false;
    }
}

void System_Vitality::Cleanup()
{
    m_Deps.State_Vitality.Cleanup();

    m_Deps.System_Logs.Log("[Vitality] INFO:"
        " Cleanup completed.");
}