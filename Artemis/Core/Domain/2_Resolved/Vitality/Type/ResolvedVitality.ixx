export module Resolved.Vitality.Type:Vitality;

import Resolved.World.Type;
import std;

namespace
{
    using AimAnchor = Resolved::World::Type::ModelLink::Anchor;
    using AimAnchorSource = Resolved::World::Type::ModelLink::AnchorSource;
}

export namespace Resolved::Vitality::Type::Vitality
{
    enum class Kind : std::uint8_t
    {
        Normal,
        Shield,
    };

    // Damage passed on to another section when this one takes damage
    struct Transfer
    {
        int TargetSection{ -1 };
        float Amount{ 0.0f };
        std::uint32_t Flags{ 0 };
    };

    struct Section
    {
        std::uint32_t NameId{ 0 };

        int SectionIndex{ -1 };
        int CollRegion{ -1 };

        float VitalityPercentage{ 0.0f };

        // Roles read from the hlmt flags
        bool IsCritical{ false };            // Kills Object (or No Player Solo)
        bool DestroysObject{ false };        // Destroys Object
        bool IsHeadshot{ false };

        Kind Kind{ Kind::Normal };

        float StunTime{ 0.0f };
        float RechargeTime{ 0.0f };

        std::vector<Transfer> Transfers{};

        // Where to aim for this section: (mode node, local offset)
        AimAnchor Aim{};
    };

    struct Vitality
    {
        std::vector<Section> Sections{};

        // Section indices by role (a model can have several, or none)
        std::vector<int> KillSections{};
        std::vector<int> HeadshotSections{};
        std::vector<int> ShieldSections{};

        // Center of the render model, fallback when a section has no anchor
        AimAnchor ObjectCenter{};

        // First match of the lists above (-1 if none). Kept for existing consumers.
        int CriticalSection{ -1 };
        int ShieldSection{ -1 };

        float MaximumVitality{ 0.0f };
        float MaximumShieldVitality{ 0.0f };
    };
}