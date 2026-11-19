export module DevTools.Viewer.Inspector.System:Loadout;

import Perception.Players.Type;
import Bridge.Codec.Tick.Type;
import std;

export namespace DevTools::Viewer::Inspector::System
{
    // Draws the loadout section: weapons, armor ability, grenades and objective of the live biped of a player.
    class LoadoutSection
    {
    private:
        using Tick = Bridge::Codec::Tick::Type::Tick;
        using Loadout = Perception::Players::Type::Loadout;

    public:
        LoadoutSection() = default;
        ~LoadoutSection() = default;

        // note: The weapon in the hands is marked on its slot.
        static auto Draw(const Tick& tick, const Loadout& loadout) -> void;
    };
}