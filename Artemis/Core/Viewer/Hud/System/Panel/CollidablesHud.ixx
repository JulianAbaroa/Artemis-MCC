export module Viewer.Hud.System:Collidables;

import Environment.Collidable.State;
import std;

export namespace Viewer::Hud::System
{
    // Draws the detail panel of the selected collidable.
    class CollidablesHud
    {
    private:
        using Collidables = Environment::Collidable::State::Collidables;

    public:
        CollidablesHud() = default;
        ~CollidablesHud() = default;

        // Draws the transform, the world mesh, the flags and the region states of the selected collidable.
        // param collidables: The debug collidables, not part of the Tick.
        // param handle: The handle of the selected object.
        static auto Draw(const std::shared_ptr<const Collidables>& collidables, std::uint32_t handle) -> void;
    };
}