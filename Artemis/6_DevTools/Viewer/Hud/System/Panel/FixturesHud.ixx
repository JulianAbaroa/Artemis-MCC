export module DevTools.Viewer.Hud.System:Fixtures;

import Perception.Details.Type;
import Perception.Entities.Type;
import Bridge.Codec.Tick.Type;
import std;

export namespace DevTools::Viewer::Hud::System
{
    // Draws the detail panel of the selected fixture.
    class FixturesHud
    {
    private:
        using Tick = Bridge::Codec::Tick::Type::Tick;

    public:
        FixturesHud() = default;
        ~FixturesHud() = default;

        // Draws the data of the selected fixture of the map.
        // param handle: The handle of the selected object.
        static auto Draw(const Tick& tick, std::uint32_t handle) -> void;
    };
}