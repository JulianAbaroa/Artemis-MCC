export module DevTools.Viewer.Inspector.System:Player;

import Perception.Players.Type;
import Bridge.Codec.Tick.Type;
import std;

export namespace DevTools::Viewer::Inspector::System
{
    // Draws the player section: connection, respawn, powerup, score and the traits the engine applies.
    class PlayerSection
    {
    private:
        using Tick = Bridge::Codec::Tick::Type::Tick;
        using Player = Perception::Players::Type::Player;

    public:
        PlayerSection() = default;
        ~PlayerSection() = default;

        // param player: The player of the selected biped.
        // note: The traits are a table of two columns, because PlayerTraits is not reflectable.
        static auto Draw(const Tick& tick, const Player& player) -> void;
    };
}