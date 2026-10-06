export module Viewer.Hud.System:Collidables;

import Export.Tick.Type;
import std;

export namespace Viewer::Hud::System
{
    // Draws the detail panel of the selected collidable.
    class CollidablesHud
    {
    private:
        using Tick = Export::Tick::Type::Tick;

    public:
        CollidablesHud() = default;
        ~CollidablesHud() = default;

        // Draws the transform, the world mesh and the flags of the selected collidable.
        // param handle: The handle of the selected object.
        static auto Draw(const Tick& tick, std::uint32_t handle) -> void;
    };
}