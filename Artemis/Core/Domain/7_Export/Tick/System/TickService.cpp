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

        // --- Layer 6: Egocentric ---
        tick.Self = m_SelfStore.Acquire();
        tick.Affordances = m_AffordanceStore.Acquire();
        tick.Raycasts = m_RaycastStore.Acquire();

        // --- Layer 7: Export ---
        m_TickStore.Publish(std::move(tick));
    }
}