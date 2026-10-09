export module Viewer.Style.Type;

import Common.Team.Type;

export namespace Viewer::Style::Type
{
    // RGB color with float channels from 0 to 1.
    struct Color
    {
        float R{}, G{}, B{};
    };

    inline constexpr float k_TranslucentAlpha{ 0.35f };
    inline constexpr float k_Background[4]{ 0.05f, 0.06f, 0.08f, 1.0f };

    inline constexpr Color k_Map{ 0.34f, 0.52f, 0.64f };
    inline constexpr Color k_Selected{ 1.0f, 1.0f, 1.0f };

    inline constexpr Color k_Collidable{ 0.37f, 0.55f, 0.40f };
    inline constexpr Color k_Health{ 0.84f, 0.70f, 0.36f };
    inline constexpr Color k_Affordance{ 0.66f, 0.44f, 0.72f };

    inline constexpr Color k_Obstacle{ 0.37f, 0.55f, 0.40f };
    inline constexpr Color k_Teleporter{ 0.32f, 0.66f, 0.62f };
    inline constexpr Color k_Shield{ 0.77f, 0.40f, 0.54f };
    inline constexpr Color k_Lift{ 0.36f, 0.49f, 0.77f };
    inline constexpr Color k_Destructible{ 0.80f, 0.46f, 0.32f };

    inline constexpr Color k_TeamRed{ 0.77f, 0.31f, 0.31f };
    inline constexpr Color k_TeamBlue{ 0.31f, 0.45f, 0.74f };
    inline constexpr Color k_TeamGreen{ 0.36f, 0.66f, 0.41f };
    inline constexpr Color k_TeamOrange{ 0.83f, 0.56f, 0.28f };
    inline constexpr Color k_TeamPurple{ 0.59f, 0.41f, 0.75f };
    inline constexpr Color k_TeamGold{ 0.84f, 0.71f, 0.35f };
    inline constexpr Color k_TeamBrown{ 0.59f, 0.45f, 0.33f };
    inline constexpr Color k_TeamPink{ 0.81f, 0.55f, 0.67f };
    inline constexpr Color k_TeamNeutral{ 0.55f, 0.55f, 0.55f };

    inline constexpr Color k_AimModelTarget{ 0.3f, 1.0f, 0.3f };
    inline constexpr Color k_AimHeadshotTarget{ 1.0f, 0.85f, 0.2f };
    inline constexpr Color k_AimCollRegion{ 0.2f, 0.6f, 1.0f };
    inline constexpr Color k_AimObjectCenter{ 0.6f, 0.6f, 0.6f };

    inline constexpr Color k_RayAimHitDynamic{ 1.0f, 0.0f, 0.0f };
    inline constexpr Color k_RayAimHitStatic{ 0.0f, 1.0f, 0.0f };
    inline constexpr Color k_RayAimNoHit{ 1.0f, 1.0f, 1.0f };

    inline constexpr Color k_RayPerceptionHitDynamic{ 1.0f, 0.0f, 1.0f };
    inline constexpr Color k_RayPerceptionHitStatic{ 1.0f, 1.0f, 0.0f };
    inline constexpr Color k_RayPerceptionNoHit{ 1.0f, 1.0f, 1.0f };

    inline constexpr Color k_LimitKill{ 0.90f, 0.30f, 0.30f };
    inline constexpr Color k_LimitSafe{ 0.40f, 0.85f, 0.50f };

    inline constexpr Color k_InteractionObject{ 0.95f, 0.78f, 0.24f };
    inline constexpr Color k_InteractionMelee{ 1.00f, 0.40f, 0.20f };
    inline constexpr Color k_InteractionAim{ 0.40f, 0.80f, 1.00f };

    inline constexpr Color k_VitalityHigh{ 0.45f, 0.85f, 0.50f };
    inline constexpr Color k_VitalityMid{ 0.84f, 0.70f, 0.36f };
    inline constexpr Color k_VitalityLow{ 0.90f, 0.45f, 0.30f };
    inline constexpr Color k_VitalityShield{ 0.35f, 0.65f, 1.00f };

    // param fraction: Vitality from 0 to 1. Values outside are clamped.
    // return: Low to mid to high color blended along the fraction.
    constexpr auto ColorOfVitality(float fraction) -> Color
    {
        const float t = fraction < 0.0f ? 0.0f : (fraction > 1.0f ? 1.0f : fraction);

        const Color& from = t < 0.5f ? k_VitalityLow : k_VitalityMid;
        const Color& to = t < 0.5f ? k_VitalityMid : k_VitalityHigh;
        const float u = t < 0.5f ? t * 2.0f : (t - 0.5f) * 2.0f;

        return Color{
            from.R + (to.R - from.R) * u,
            from.G + (to.G - from.G) * u,
            from.B + (to.B - from.B) * u };
    }

    // return: The color of the team. Neutral grey for neutral or unlisted teams.
    constexpr auto ColorOfTeam(Common::Team::Type::Team team) -> Color
    {
        using Team = Common::Team::Type::Team;

        switch (team)
        {
        case Team::Red:     return k_TeamRed;
        case Team::Blue:    return k_TeamBlue;
        case Team::Green:   return k_TeamGreen;
        case Team::Orange:  return k_TeamOrange;
        case Team::Purple:  return k_TeamPurple;
        case Team::Gold:    return k_TeamGold;
        case Team::Brown:   return k_TeamBrown;
        case Team::Pink:    return k_TeamPink;
        case Team::Neutral:
        default:            return k_TeamNeutral;
        }
    }
}