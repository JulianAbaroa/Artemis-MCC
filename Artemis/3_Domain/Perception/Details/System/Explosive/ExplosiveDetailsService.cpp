module Perception.Details.System;
import :ExplosiveDetails;

import Common.Semantic.Type;
import Blam.MapModel.Definitions.Type;
import Blam.MapModel.Definitions.State;
import Perception.Classifier.Type;

namespace
{
    using Common::Semantic::Type::Function;
    using Blam::MapModel::Definitions::Type::Hlmt::Hlmt;
    using Blam::MapModel::Definitions::Type::Jpt::Jpt;
    using Blam::MapModel::Definitions::State::DefinitionsStore;
    using Perception::Classifier::Type::Classified;
    using Perception::Details::Type::ExplosiveDetail;
    using Perception::Details::Type::ExplosiveDetails;

    // return: The model tag of a crate or a scenery, empty for any other tag.
    // note: Explosives are crates, except the propane tank of some maps, a scenery.
    auto ModelOf(const DefinitionsStore& definitions, const std::string& tagName) -> std::string
    {
        if (const auto* bloc{ definitions.m_Bloc.Get(tagName) }) return bloc->Base.ModelTagName;
        if (const auto* scen{ definitions.m_Scen.Get(tagName) }) return scen->Base.ModelTagName;
        return {};
    }

    // return: The damage effect of the instant response with the lowest threshold that has one, null when none has.
    // note: Measured on forge_halo (B4x): the fusion coil has two responses with the same effect (0.99 and 0), the propane tank
    // one at 0.5 without effect and one at 0 with it, the land mine and the plasma battery one at 0.
    auto ExplosionOf(const DefinitionsStore& definitions, const std::string& tagName) -> const Jpt*
    {
        const Hlmt* hlmt{ definitions.m_Hlmt.Get(ModelOf(definitions, tagName)) };
        if (hlmt == nullptr) return nullptr;

        const std::string* effect{ nullptr };
        float threshold{ std::numeric_limits<float>::max() };
        for (const auto& section : hlmt->DamageSections)
        {
            for (const auto& response : section.InstantResponses)
            {
                if (response.TransitionDamageEffectTagName.empty() || response.DamageThreshold >= threshold) continue;

                effect = &response.TransitionDamageEffectTagName;
                threshold = response.DamageThreshold;
            }
        }

        return effect != nullptr ? definitions.m_Jpt.Get(*effect) : nullptr;
    }
}

namespace Perception::Details::System
{
    auto ExplosiveDetailsService::Update() -> void
    {
        auto classifiedsPtr = m_ClassifierStore.Acquire();
        auto objectTablePtr = m_ObjectStore.Acquire();
        if (!classifiedsPtr || !objectTablePtr) return;

        const bool hasDefinitions{ m_DefinitionsStore.m_Hlmt.IsFrozen() };
        ExplosiveDetails explosives{};

        for (const Classified& classified : *classifiedsPtr)
        {
            if (classified.Function != Function::Explosive) continue;

            const auto objectIt{ objectTablePtr->find(classified.Handle) };
            if (objectIt == objectTablePtr->end()) continue;

            ExplosiveDetail detail{ .Handle = classified.Handle };
            const Jpt* jpt{ hasDefinitions ? ExplosionOf(m_DefinitionsStore, objectIt->second.TagName) : nullptr };
            if (jpt != nullptr)
            {
                detail.RadiusMin = jpt->RadiusMin;
                detail.RadiusMax = jpt->RadiusMax;
                detail.Damage = jpt->DamageUpperBoundMax;
            }

            explosives.emplace(classified.Handle, detail);
        }

        m_ExplosiveDetailsStore.Publish(std::move(explosives));
    }

    auto ExplosiveDetailsService::Cleanup() -> void
    {
        m_ExplosiveDetailsStore.Cleanup();

        m_Log.Message("[ExplosiveDetailsService] INFO: Cleanup completed.");
    }
}