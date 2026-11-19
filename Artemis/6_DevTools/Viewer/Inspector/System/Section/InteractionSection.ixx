export module DevTools.Viewer.Inspector.System:Interaction;

import Views.Affordance.Type;
import Bridge.Codec.Tick.Type;
import std;

export namespace DevTools::Viewer::Inspector::System
{
    // Draws the interaction section: what the local player can do with the selected object.
    class InteractionSection
    {
    private:
        using Tick = Bridge::Codec::Tick::Type::Tick;
        using Affordance = Views::Affordance::Type::Affordance;

    public:
        InteractionSection() = default;
        ~InteractionSection() = default;

        // return: The affordance of the object, or null when it has none.
        static auto Find(const Tick& tick, std::uint32_t handle) -> const Affordance*;

        // Draws the affordance of the selected object: classification, spatial data, seats and child vehicles.
        // param item: The affordance of the object. Null shows that there is none.
        static auto Draw(const Tick& tick, const Affordance* item) -> void;
    };
}