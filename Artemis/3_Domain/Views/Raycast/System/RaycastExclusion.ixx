export module Views.Raycast.System:Exclusion;

import Perception.Entities.Type;
import Perception.ObjectGraph.Type;
import Perception.Players.Type;
import Views.Raycast.Type;
import std;

// What the rays of an agent ignore, built from the object graph for every player at once (F4.B, design of the views per agent).

namespace
{
    using Perception::Entities::Type::Entities;
    using Perception::ObjectGraph::Type::ObjectNodes;
    using Perception::Players::Type::Players;
    using Views::Raycast::Type::PlayerExclusions;
}

export namespace Views::Raycast::System
{
    // return: The vehicle at the root of the parent chain of a biped, or k_NoHandle when it rides none.
    // note: A turret is a vehicle attached to another one, so the root is the vehicle that carries every part.
    auto FindVehicleRoot(const Entities& entities, std::uint32_t bipedHandle) -> std::uint32_t;

    // Builds the exclusion of every player in one pass over the players: its biped, its loadout (if it is alive), the children of the
    // biped in the graph, and, if it rides a vehicle, the root of the vehicle and its vehicle children.
    // return: One exclusion per row of players, in the same order. A player without biped gets an empty one.
    // note: The same set as BuildSelfExclusion for each row, without walking the object table or the entities.
    auto BuildGraphExclusions(const Players& players, const Entities& entities, const ObjectNodes& nodes) -> PlayerExclusions;
}