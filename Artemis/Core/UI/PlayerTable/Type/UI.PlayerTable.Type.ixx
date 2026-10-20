export module UI.PlayerTable.Type;

import Tables.Player.Type;
import Export.Tick.Type;
import std;

namespace
{
    using Tables::Player::Type::Alive::AlivePlayer;
}

export namespace UI::PlayerTable::Type
{
    // Alive players sorted by handle.
    struct Players : std::vector<const AlivePlayer*>
    {
        using vector::vector;
    };
}