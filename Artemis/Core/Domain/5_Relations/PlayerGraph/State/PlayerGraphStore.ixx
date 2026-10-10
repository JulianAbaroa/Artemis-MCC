export module Relations.PlayerGraph.State;

import Common.State.System;
import Relations.PlayerGraph.Type;
import std;

export namespace Relations::PlayerGraph::State
{
    using PlayerTrees = std::vector<Relations::PlayerGraph::Type::PlayerTree>;
    using PlayerGraphStore = Common::State::System::Snapshot<PlayerTrees>;
}