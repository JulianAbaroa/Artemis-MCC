export module DevTools.Viewer.Hud.System:Affordances;

import Blam.Runtime.Interaction.Type;
import Blam.Runtime.Object.Type;
import Views.Affordance.Type;
import Bridge.Codec.Tick.Type;
import std;

export namespace DevTools::Viewer::Hud::System
{
    // Draws the detail panel of the selected affordance.
    class AffordancesHud
    {
    private:
        using Tick = Bridge::Codec::Tick::Type::Tick;

    public:
        AffordancesHud() = default;
        ~AffordancesHud() = default;

        // Draws the affordances, seats and interactions of the selected object.
        // param handle: The handle of the selected object.
        static auto Draw(const Tick& tick, std::uint32_t handle) -> void;
    };
}