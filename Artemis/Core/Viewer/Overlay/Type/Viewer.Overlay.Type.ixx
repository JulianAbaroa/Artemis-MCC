export module Viewer.Overlay.Type;

import std;

export namespace Viewer::Overlay::Type
{
    // What the map and the HUD highlight. Count is the number of modes and is not a mode.
    enum class Mode : std::uint8_t
    {
        Default = 0,
        Collidable,
        Health,
        Fixture,
        Affordance,

        Count
    };
}