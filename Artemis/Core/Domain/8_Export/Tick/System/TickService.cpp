module Export.Tick.System;

import Egocentric.Self.Type;
import Export.Tick.Type;

namespace
{
    using Export::Tick::Type::Tick;
    using Export::Tick::Type::AliveInteraction;

    using Egocentric::Self::Type::k_SelfLocalIndex;
}

namespace Export::Tick::System
{
    auto TickService::Assemble(std::uint64_t generation) -> void
    {
        ::Tick tick;
        tick.Generation = generation;

        if (m_DefinitionsStore.m_Sbsp.IsFrozen())
        {
            if (!m_Map)
            {
                m_Map = std::shared_ptr<const std::unordered_map<std::string, Resolved::Definitions::Type::Sbsp::Sbsp>>(
                    std::shared_ptr<void>{}, &m_DefinitionsStore.m_Sbsp.All());
            }

            if (!m_Limits && m_DefinitionsStore.m_Scnr.IsFrozen())
            {
                m_Limits = std::shared_ptr<const std::unordered_map<std::string, Resolved::Definitions::Type::Scnr::Scnr>>(
                    std::shared_ptr<void>{}, &m_DefinitionsStore.m_Scnr.All());
            }

            if (!m_Designs && m_DefinitionsStore.m_Sddt.IsFrozen())
            {
                m_Designs = std::shared_ptr<const std::unordered_map<std::string, Resolved::Definitions::Type::Sddt::Sddt>>(
                    std::shared_ptr<void>{}, &m_DefinitionsStore.m_Sddt.All());
            }
        }
        else
        {
            m_Map.reset();
            m_Limits.reset();
            m_Designs.reset();
        }

        tick.Map = m_Map;
        tick.Limits = m_Limits;
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
        tick.Aims = m_AimStore.Acquire();

        // --- Layer 6: Egocentric ---
        tick.Self = m_SelfStore.Acquire();
        tick.Affordances = m_AffordanceStore.Acquire();
        tick.Raycasts = m_RaycastStore.Acquire();

        // --- Layer 7: Export ---
        m_TickStore.Publish(std::move(tick));
    }
}