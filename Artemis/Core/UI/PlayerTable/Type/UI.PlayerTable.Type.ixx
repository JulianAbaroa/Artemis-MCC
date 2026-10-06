export module UI.PlayerTable.Type;

import Export.Tick.Type;
import std;

namespace
{
    using AlivePlayer = Export::Tick::Type::AlivePlayer;
}

export namespace UI::PlayerTable::Type
{
    // Alive players sorted by handle.
    using Players = std::vector<const AlivePlayer*>;
}