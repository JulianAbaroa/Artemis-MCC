export module Resolved.Vitality.Type:Vitality;

import Resolved.World.Type;
import std;

namespace
{
    using Resolved::World::Type::ModelLink::Anchor;
}

export namespace Resolved::Vitality::Type::Vitality
{
    enum class Kind : std::uint8_t
    {
        Normal,
        Shield,
    };

    // Damage passed on to another section when this one takes damage.
    struct Transfer
    {
        int TargetSection{ -1 };
        float Amount{};
        std::uint32_t Flags{};
    };

    // One damage section of a model, with the roles read from its flags.
    struct Section
    {
        std::uint32_t NameId{};

        int SectionIndex{ -1 };
        int CollRegion{ -1 };

        float VitalityPercentage{};

        // Roles read from the hlmt flags.
        bool IsCritical{};
        bool DestroysObject{};
        bool IsHeadshot{};

        Kind Kind{ Kind::Normal };

        float StunTime{};
        float RechargeTime{};

        std::vector<Transfer> Transfers{};

        // Where to aim for this section: mode node and local offset.
        Anchor Aim{};
    };

    // Vitality layout of a model: its damage sections and which of them kill, shield or take headshots.
    struct Vitality
    {
        std::vector<Section> Sections{};

        // Section indices by role. A model can have several, or none.
        std::vector<int> KillSections{};
        std::vector<int> HeadshotSections{};
        std::vector<int> ShieldSections{};

        // Center of the render model, fallback when a section has no anchor.
        Anchor ObjectCenter{};

        // First match of the lists above, or -1 if none.
        int CriticalSection{ -1 };
        int ShieldSection{ -1 };

        float MaximumVitality{};
        float MaximumShieldVitality{};
    };
}