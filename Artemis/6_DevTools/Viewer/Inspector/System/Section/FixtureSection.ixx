export module DevTools.Viewer.Inspector.System:Fixture;

import Perception.Details.Type;
import Perception.Entities.Type;
import Bridge.Codec.Tick.Type;
import std;

export namespace DevTools::Viewer::Inspector::System
{
    // Draws the fixture section of the selected object: teleporter, lift, barrier, zone, prop, spawn point or objective item.
    class FixtureSection
    {
    private:
        using Tick = Bridge::Codec::Tick::Type::Tick;

    public:
        FixtureSection() = default;
        ~FixtureSection() = default;

        // return: Whether the object is a fixture of the map, so the section has something to show.
        static auto Applies(const Tick& tick, std::uint32_t handle) -> bool;

        // Draws the data of the selected fixture of the map.
        // param handle: The handle of the selected object.
        static auto Draw(const Tick& tick, std::uint32_t handle) -> void;
    };
}