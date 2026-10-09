module Export.Tick.System;

import Export.Tick.Type;

namespace
{
    using Tick = Export::Tick::Type::Tick;
}

namespace Export::Tick::System
{
    auto TickService::Assemble(std::uint64_t generation) -> void
    {
        ::Tick tick;
        tick.Generation = generation;

        if (m_DefinitionsStore.Sbsp.IsFrozen())
        {
            if (!m_Map)
            {
                m_Map = std::shared_ptr<const Export::Tick::Type::MapSbsps>(
                    std::shared_ptr<void>{}, &m_DefinitionsStore.Sbsp.All());
            }

            if (!m_Limits && m_DefinitionsStore.Scnr.IsFrozen())
            {
                m_Limits = std::shared_ptr<const Export::Tick::Type::MapScnrs>(
                    std::shared_ptr<void>{}, &m_DefinitionsStore.Scnr.All());
            }

            if (!m_Designs && m_DefinitionsStore.Sddt.IsFrozen())
            {
                m_Designs = std::shared_ptr<const Export::Tick::Type::MapSddts>(
                    std::shared_ptr<void>{}, &m_DefinitionsStore.Sddt.All());
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
        tick.Interaction = m_InteractionStore.Acquire();

        // --- Layer 4: Relations ---
        tick.Classifieds = m_ClassifierStore.Acquire();
        tick.ObjectGraph = m_ObjectGraphStore.Acquire();
        tick.PlayerGraph = m_PlayerGraphStore.Acquire();

        // --- Layer 5: Environment ---
        tick.Collidables = m_CollidableStore.Acquire();
        tick.Fixtures = m_FixturesStore.Acquire();
        tick.Healths = m_HealthStore.Acquire();
		tick.Aims = m_AimStore.Acquire();

        // --- Layer 6: Egocentric ---
        tick.Self = m_SelfStore.Acquire();
        tick.Affordances = m_AffordanceStore.Acquire();
        tick.Raycasts = m_RaycastStore.Acquire();

        // --- Layer 7: Export ---
        m_TickStore.Publish(std::move(tick));
    }
}