export module Viewer.Hud.System:Affordances;

import Tables.Interaction.Type;
import Tables.Object.Type;
import Egocentric.Affordance.Type;
import Export.Tick.Type;
import std;

export namespace Viewer::Hud::System
{
    // Draws the detail panel of the selected affordance.
    class AffordancesHud
    {
    private:
        using Tick = Export::Tick::Type::Tick;

    public:
        AffordancesHud() = default;
        ~AffordancesHud() = default;

        // Draws the affordances, seats and interactions of the selected object.
        // param handle: The handle of the selected object.
        static auto Draw(const Tick& tick, std::uint32_t handle) -> void;
    };
}