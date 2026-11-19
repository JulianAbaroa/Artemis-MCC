export module DevTools.Viewer.Inspector.System:Vitality;

import Perception.Details.Type;
import Bridge.Codec.Tick.Type;
import std;

export namespace DevTools::Viewer::Inspector::System
{
    // Draws the vitality section of the selected object: the maximums, the damage sections and their current values.
    class VitalitySection
    {
    private:
        using Tick = Bridge::Codec::Tick::Type::Tick;
        using VitalityDetail = Perception::Details::Type::VitalityDetail;

    public:
        VitalitySection() = default;
        ~VitalitySection() = default;

        // return: The vitality detail of the object, or null when it has none.
        static auto Find(const Tick& tick, std::uint32_t handle) -> const VitalityDetail*;

        // param handle: The handle of the selected object.
        static auto Draw(const Tick& tick, std::uint32_t handle, const VitalityDetail& detail) -> void;
    };
}