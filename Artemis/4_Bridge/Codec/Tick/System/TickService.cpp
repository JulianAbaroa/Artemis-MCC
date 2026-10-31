module Bridge.Codec.Tick.System;

import Views.Self.Type;
import Bridge.Codec.Tick.Type;

namespace
{
    using Bridge::Codec::Tick::Type::Tick;
    using Blam::Runtime::Interaction::Type::Alive::AliveInteraction;

    using Views::Self::Type::k_SelfLocalIndex;
}

namespace Bridge::Codec::Tick::System
{
    auto TickService::Assemble(std::uint64_t generation) -> void
    {
        ::Tick tick;
        tick.Generation = generation;

        if (m_DefinitionsStore.m_Sbsp.IsFrozen())
        {
            if (!m_Map)
            {
                m_Map = std::shared_ptr<const std::unordered_map<std::string, Blam::MapModel::Definitions::Type::Sbsp::Sbsp>>(
                    std::shared_ptr<void>{}, &m_DefinitionsStore.m_Sbsp.All());
            }

            if (!m_Limits && m_DefinitionsStore.m_Scnr.IsFrozen())
            {
                m_Limits = std::shared_ptr<const std::unordered_map<std::string, Blam::MapModel::Definitions::Type::Scnr::Scnr>>(
                    std::shared_ptr<void>{}, &m_DefinitionsStore.m_Scnr.All());
                // note: A map has one scnr. With more, the first one gives the id (D56).
                m_MapId = m_Limits->empty() ? 0 : m_Limits->begin()->second.MapId;
            }

            if (!m_Designs && m_DefinitionsStore.m_Sddt.IsFrozen())
            {
                m_Designs = std::shared_ptr<const std::unordered_map<std::string, Blam::MapModel::Definitions::Type::Sddt::Sddt>>(
                    std::shared_ptr<void>{}, &m_DefinitionsStore.m_Sddt.All());
            }
        }
        else
        {
            m_Map.reset();
            m_Limits.reset();
            m_MapId = 0;
            m_Designs.reset();
        }

        tick.Map = m_Map;
        tick.Limits = m_Limits;
        tick.MapId = m_MapId;
        tick.Designs = m_Designs;

        // --- Layer 3: Tables ---
        tick.ObjectTable = m_ObjectStore.Acquire();
        tick.PlayerTable = m_PlayerStore.Acquire();
        // note: Shares the published interactions and points to the one of the agent.
        const auto interactions{ m_InteractionStore.Acquire() };
        tick.Interaction = interactions
            ? std::shared_ptr<const AliveInteraction>(interactions, &(*interactions)[k_SelfLocalIndex])
            : nullptr;

        // --- Layer 4: Relations ---
        tick.Classifieds = m_ClassifierStore.Acquire();
        tick.ObjectGraph = m_ObjectGraphStore.Acquire();

        // --- Layer 5: Environment ---
        tick.Entities = m_EntitiesStore.Acquire();
        tick.Players = m_PlayersStore.Acquire();
        tick.VitalityDetails = m_VitalityDetailsStore.Acquire();
        tick.ZoneDetails = m_ZoneDetailsStore.Acquire();
        tick.TeleporterDetails = m_TeleporterDetailsStore.Acquire();
        tick.LiftDetails = m_LiftDetailsStore.Acquire();
        tick.BarrierDetails = m_BarrierDetailsStore.Acquire();
        tick.ObjectiveDetails = m_ObjectiveDetailsStore.Acquire();
        tick.ProjectileDetails = m_ProjectileDetailsStore.Acquire();
        tick.WeaponDetails = m_WeaponDetailsStore.Acquire();
        tick.Aims = m_AimStore.Acquire();
        tick.Events = m_EventsStore.Acquire();
        tick.Match = m_MatchStore.Acquire();

        // --- Layer 6: Egocentric ---
        tick.Self = m_SelfStore.Acquire();
        tick.Affordances = m_AffordanceStore.Acquire();
        tick.Raycasts = m_RaycastStore.Acquire();

        // --- Layer 7: Export ---
        m_TickStore.Publish(std::move(tick));
    }
}