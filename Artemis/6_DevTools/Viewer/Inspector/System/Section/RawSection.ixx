export module DevTools.Viewer.Inspector.System:Raw;

import Perception.Collidable.Type;
import Bridge.Codec.Tick.Type;
import std;

export namespace DevTools::Viewer::Inspector::System
{
    // Draws the raw section: the engine data as it is, for checking the other sections against it.
    class RawSection
    {
    private:
        using Tick = Bridge::Codec::Tick::Type::Tick;
        using Collidable = Perception::Collidable::Type::Collidable;

    public:
        RawSection() = default;
        ~RawSection() = default;

        // Draws the region states and the raw region block of the collidable, and the engine interaction state of the local player.
        // param collidable: The collidable of the selected object. Null when it is not one.
        // param isLocalBiped: Whether the selected object is the biped of the local player, which the interaction state belongs to.
        static auto Draw(const Tick& tick, const Collidable* collidable, bool isLocalBiped) -> void;
    };
}