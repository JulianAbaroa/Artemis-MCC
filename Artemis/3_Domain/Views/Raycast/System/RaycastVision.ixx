export module Views.Raycast.System:Vision;

import Blam.MapModel.World.System;
import Views.Raycast.Type;
import std;

// Who sees whom in one tick: a cone and one head to head ray per pair, shared by all the agents (D310, F4.3). Internal to Views: not in the
// public interface.

namespace
{
    using Blam::MapModel::World::System::Raycaster;
    using Views::Raycast::Type::PlayerExclusions;
    using Views::Raycast::Type::RaycastScene;
    using Views::Raycast::Type::SightCounters;
    using Views::Raycast::Type::SightPlayers;
    using Views::Raycast::Type::SightSeen;
}

export namespace Views::Raycast::System
{
    // Answers who sees whom among the players with a head, with the range and the cone of Views.Raycast.Type:Sight.
    // A pair is a candidate when it is within the range and at least one of its two players is a viewer that has the other inside its cone.
    // One ray goes from the head of the first to the head of the second over the prepared scene, ignoring what both players carry or ride, and
    // serves both directions: the line is free when nothing is hit before the second head. A player sees another when the other is in its cone
    // and the line is free.
    // param exclusions: The exclusion of each row of the players. A row past its end excludes nothing.
    // param counters: Receives the candidate pairs and the rays of this pass.
    // return: For each row a, which rows b it sees. Only viewers see.
    auto ComputeSight(const Raycaster& raycaster, const RaycastScene& scene, const PlayerExclusions& exclusions, const SightPlayers& players,
        SightCounters& counters) -> SightSeen;
}