export module Viewer.Hud.System:Fixtures;

import Export.Tick.Type;
import std;

export namespace Viewer::Hud::System
{
    // Draws the detail panel of the selected fixture.
    class FixturesHud
    {
    private:
        using Tick = Export::Tick::Type::Tick;

    public:
        FixturesHud() = default;
        ~FixturesHud() = default;

        // Draws the data of the selected fixture of the map.
        // param handle: The handle of the selected object.
        static auto Draw(const Tick& tick, std::uint32_t handle) -> void;
    };
}