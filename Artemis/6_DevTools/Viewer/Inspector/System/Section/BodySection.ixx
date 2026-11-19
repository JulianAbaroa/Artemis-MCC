export module DevTools.Viewer.Inspector.System:Body;

import Perception.Players.Type;
import Bridge.Codec.Tick.Type;
import std;

export namespace DevTools::Viewer::Inspector::System
{
    // Draws the body section: zoom, last damage, what the feet touch, crouch and velocity of the live biped of a player.
    class BodySection
    {
    private:
        using Tick = Bridge::Codec::Tick::Type::Tick;
        using Body = Perception::Players::Type::Body;

    public:
        BodySection() = default;
        ~BodySection() = default;

        static auto Draw(const Tick& tick, const Body& body) -> void;
    };
}