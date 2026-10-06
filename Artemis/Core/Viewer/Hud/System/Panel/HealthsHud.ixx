export module Viewer.Hud.System:Vitalities;

import Export.Tick.Type;
import std;

export namespace Viewer::Hud::System
{
    // Draws the detail panel of the selected health.
    class HealthsHud
    {
    private:
        using Tick = Export::Tick::Type::Tick;

    public:
        HealthsHud() = default;
        ~HealthsHud() = default;

        // Draws the health, the shields and the vitality sections of the selected object.
        // param handle: The handle of the selected object.
        static auto Draw(const Tick& tick, std::uint32_t handle) -> void;
    };
}