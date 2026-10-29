export module DevTools.Workbench.PlayerTable.Type;

import Blam.Runtime.Player.Type;
import Bridge.Codec.Tick.Type;
import std;

namespace
{
    using Blam::Runtime::Player::Type::Alive::AlivePlayer;
}

export namespace DevTools::Workbench::PlayerTable::Type
{
    // Alive players sorted by handle.
    struct Players : std::vector<const AlivePlayer*>
    {
        using vector::vector;
    };
}