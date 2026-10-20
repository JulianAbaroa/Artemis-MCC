export module Environment.Details.Type:Vitality;

import Resolved.Vitality.Type;
import std;

namespace
{
    using Resolved::Vitality::Type::Vitality::Vitality;
}

export namespace Environment::Details::Type
{
    // Damage section whose vitality kills the object at 0 (D85).
    // note: The sections the layout marks critical are not reliable: the headshot section of a Spartan stays at 1 after a body kill,
    // and the Ghost, the Falcon and the plasma battery have none.
    constexpr std::size_t k_LifeSection{ 0 };

    // Vitality of every damage section of one entity with HasVitality (VitalityDetails, D84, D87).
    // note: Used by Entities for the lifecycle, by the panels of the viewer and later by Players (shield and health).
    struct VitalityDetail
    {
        std::uint32_t Handle{};

        // Vitality of each section, from 0 to 1, in the order of the layout. Index i is damage section i of the datum.
        // note: Never empty, so k_LifeSection is always valid.
        std::vector<float> Sections{};

        // Layout of the tag: names, kinds and roles of the sections.
        std::shared_ptr<const ::Vitality> Layout{};
    };

    struct VitalityDetails : std::unordered_map<std::uint32_t, VitalityDetail>
    {
        using unordered_map::unordered_map;
    };
}