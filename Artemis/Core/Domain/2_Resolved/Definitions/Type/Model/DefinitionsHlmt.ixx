export module Resolved.Definitions.Type:Hlmt;

import std;

export namespace Resolved::Definitions::Type::Hlmt
{
    struct InstantResponse
    {
        std::uint32_t Flags{};
    };

    struct DamageTransfer
    {
        std::uint32_t Flags{};
        float TransferAmount{};
        std::int32_t DamageSectionIndex{ -1 };
    };

    struct DamageSection
    {
        std::uint32_t Name{};
        std::uint32_t Flags{};

        float VitalityPercentage{};

        // Non-zero when this section is backed by a shield material
        std::uint32_t ShieldGlobalMaterialName{};

        float StunTime{};
        float RechargeTime{};

        std::vector<InstantResponse> InstantResponses{};
		std::vector<DamageTransfer> SectionDamageTransfers{};
    };

    struct ModelTarget
    {
        std::uint32_t MarkerName{};

        float Size{};
        float ConeAngle{};

		std::int32_t DamageSectionIndex{ -1 };
		std::int32_t VariantIndex{ -1 };

        float TargetingRelevance{};

        // Headshot = bit 0
        std::uint32_t LockOnFlags{};
    };

    struct CollisionRegion
    {
        std::uint32_t Name{};
		std::int32_t CollisionRegionIndex{ -1 };
        std::int32_t PhysicsRegionIndex{ -1 };
    };

    struct Hlmt
    {
        std::string TagName{};

        std::string RenderModelTagName{};
        std::string CollisionModelTagName{};

        float MaximumVitality{};
        float MaximumShieldVitality{};

        std::int32_t ShieldedStateDamageSectionIndex{ -1 };
        std::vector<DamageSection> DamageSections{};
		std::vector<ModelTarget> ModelTargets{};
		std::vector<CollisionRegion> CollisionRegions{};
    };
}