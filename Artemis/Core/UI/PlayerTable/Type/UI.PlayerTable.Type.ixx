export module UI.PlayerTable.Type;

import Export.Tick.Type;
import std;

namespace
{
    using Export::Tick::Type::AlivePlayer;
}

export namespace UI::PlayerTable::Type
{
    // Alive players sorted by handle.
    struct Players : std::vector<const AlivePlayer*>
    {
        using vector::vector;
    };
}