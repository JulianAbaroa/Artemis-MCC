#include "pch.h"

#include "System_Tick.h"

#include "Core/States/Tick/State_Tick.h"
#include "Core/States/Sources/Tables/Object/State_ObjectTable.h"
#include "Core/States/Sources/Tables/Player/State_PlayerTable.h"
#include "Core/States/Sources/Tables/Interaction/State_InteractionTable.h"
#include "Core/States/Structure/Classifier/State_Classifier.h"
#include "Core/States/Structure/Graph/Object/State_ObjectGraph.h"
#include "Core/States/Structure/Graph/Player/State_PlayerGraph.h"
#include "Core/States/Environment/Collidables/State_Collidables.h"
#include "Core/States/Environment/Fixtures/State_Fixtures.h"
#include "Core/States/Environment/Vitality/State_Vitality.h"
#include "Core/States/Egocentric/Self/State_Self.h"
#include "Core/States/Egocentric/Affordances/State_Affordances.h"

void System_Tick::Assemble(uint64_t generation)
{
    Tick tick;
    tick.Generation = generation;

    // --- Layer 0: Sources ---
    tick.ObjectTable = m_Deps.State_ObjectTable.Acquire();
    tick.PlayerTable = m_Deps.State_PlayerTable.Acquire();
    tick.Interaction = m_Deps.State_InteractionTable.Acquire();

    // --- Layer 1: Structure ---
    tick.Classifieds = m_Deps.State_Classifier.Acquire();
    tick.ObjectGraph = m_Deps.State_ObjectGraph.Acquire();
    tick.PlayerGraph = m_Deps.State_PlayerGraph.Acquire();

    // --- Layer 2: Environment ---
    tick.Collidables = m_Deps.State_Collidables.Acquire();
    tick.Fixtures = m_Deps.State_Fixtures.Acquire();
    tick.Vitalities = m_Deps.State_Vitality.Acquire();

    // --- Layer 3: Egocentric ---
    tick.Self = m_Deps.State_Self.Acquire();
    tick.Affordances = m_Deps.State_Affordances.Acquire();

    m_Deps.State_Tick.Publish(std::move(tick));
}