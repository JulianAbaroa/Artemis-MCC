export module Views.View.Type;

import Common.Engine.Type;
import Common.Math.Type;
import Common.Semantic.Type;
import Common.Team.Type;
import Blam.Runtime.Object.Type;
import Perception.Players.Type;
import Views.Raycast.Type;
import std;

namespace
{
    using Common::Math::Type::Vec3;
    using Common::Semantic::Type::Event;
    using Common::Semantic::Type::FlagState;
    using Common::Semantic::Type::Function;
    using Common::Semantic::Type::Subtype::Powerup;
    using Common::Team::Type::Team;
    using Blam::Runtime::Object::Type::Biped::GrenadeType;
    using Blam::Runtime::Object::Type::Biped::ZoomLevel;
    using Perception::Players::Type::GroundKind;
    using Views::Raycast::Type::HitKind;

    using Common::Engine::Type::k_MaxPlayers;
}

// What one agent can know (F4, D307-D321). Every field is in world space and world units. A View that is not computed is all zero.

export namespace Views::View::Type
{
    // Rays of the perception pattern of a View.
    inline constexpr std::size_t k_ViewRayCount{ 16 };

    // Flags of a View: [0] is the one of the team of the agent in this round, [1] the rival one.
    inline constexpr std::size_t k_ViewFlags{ 2 };

    // Sounds and notices a View keeps.
    inline constexpr std::size_t k_ViewSounds{ 8 };
    inline constexpr std::size_t k_ViewNotices{ 8 };

    // Slots of the weapons of a Self: the primary and the secondary one.
    inline constexpr std::size_t k_ViewWeaponSlots{ 2 };

    // How much of the world the agent sees (D321). Effective value, written in every View.
    // note: Full sees every living player without cone or rays. HumanLike sees through the cone and the line of sight (D310).
    enum class ViewMode : std::uint8_t
    {
        Full = 0,
        HumanLike = 1,
    };

    // Players that get a View, by player table index (the low 16 bits of the handle). It enters the pipeline as data (D307): the same
    // Snapshot with the same AgentSet gives the same Views.
    struct AgentSet
    {
        std::uint32_t Mask{ 0 };

        // Mode of every View of the set (D321). The pipeline passes HumanLike until the bridge carries the choice (F4.8).
        ViewMode Mode{ ViewMode::HumanLike };

        // return: The set with every player.
        static auto All() -> AgentSet
        {
            return AgentSet{ .Mask = 0xFFFFFFFFu };
        }

        // return: Whether the player of the handle is in the set.
        auto Contains(std::uint32_t playerHandle) const -> bool
        {
            const std::uint32_t index{ playerHandle & 0xFFFFu };
            return index < 32 && ((this->Mask >> index) & 1u) != 0;
        }

        // Adds the player of the handle.
        auto Add(std::uint32_t playerHandle) -> void
        {
            const std::uint32_t index{ playerHandle & 0xFFFFu };
            if (index < 32) this->Mask |= (1u << index);
        }
    };

    // Relation of the agent to what it looks at or to another player, by the team of the player involved.
    enum class ViewRelation : std::uint8_t
    {
        None = 0,
        Self = 1,
        Ally = 2,
        Enemy = 3,
        Neutral = 4,
    };

    // Bits of ViewOther.Channels: how the agent knows of the other player.
    enum class ViewChannel : std::uint8_t
    {
        Hud = 1,
        Seen = 2,
        Radar = 4,
    };

    enum class ViewSoundKind : std::uint8_t
    {
        Fire = 0,
        Explosion = 1,
    };

    // Side of a flag in a notice, from the team of the agent.
    enum class ViewFlagSide : std::uint8_t
    {
        None = 0,
        Mine = 1,
        Rival = 2,
    };

    struct ViewHead
    {
        std::uint32_t PlayerHandle{};

        // Row of the player in Players.
        std::uint8_t PlayerRow{};

        // Split-screen user of the player, 0xFF when it is not a local one.
        std::uint8_t LocalIndex{};

        // Handle of the biped of the player, 0xFFFFFFFF without one.
        std::uint32_t BipedHandle{};
        ViewMode Mode{};
        bool IsComputed{};
        bool HasBody{};
        ::Team Team{};

        // BaseTeam of the stand that the team owns in this round, Neutral when it owns none.
        ::Team Side{};
    };

    // A weapon of the agent. Repeats what the Bridge gives on purpose, so an agent builds its observation without joins.
    struct ViewWeapon
    {
        std::uint32_t Handle{};
        std::uint8_t Subtype{};
        bool IsEquipped{};

        // Rounds of the first magazine, 0 on an energy weapon.
        std::uint16_t Loaded{};
        std::uint16_t Reserve{};
        float Energy{};
        float Heat{};
        bool IsOverheated{};
        bool IsReloading{};
        float ChargeFraction{};
        bool CanFireNow{};
    };

    // The agent itself: a copy of Players, Entities and WeaponDetails, with the eye and the angles of the aim.
    struct ViewSelf
    {
        // Eye of the agent (D309), zero when the biped has no tag.
        Vec3 Eye{};
        Vec3 Feet{};

        // Heading and pitch of Body.Aim, radians.
        float FrameYaw{};
        float AimPitch{};

        Vec3 LinearVelocity{};
        Vec3 OwnVelocity{};

        float Vitality{};
        float Shield{};
        GroundKind Ground{};
        bool IsGrounded{};
        std::uint32_t AirborneTicks{};
        float CrouchFraction{};
        ZoomLevel Zoom{};
        std::uint32_t TicksAlive{};
        std::uint32_t RespawnTicks{};
        ::Powerup Powerup{};
        std::uint32_t PowerupTicks{};
        GrenadeType SelectedGrenade{};
        std::uint8_t FragGrenades{};
        std::uint8_t PlasmaGrenades{};
        std::uint32_t Ability{};
        float AbilityEnergy{};
        bool IsAbilityActive{};
        bool CarriesFlag{};

        // Ticks since the last damage, 0xFFFFFFFF when unknown.
        std::uint32_t TicksSinceDamage{};

        // A weapon that tracks the agent (or the vehicle it sits in) is locked on it.
        bool IsTracked{};

        std::array<ViewWeapon, k_ViewWeaponSlots> Weapons{};
    };

    // What the aim ray of the agent hits (F4.2).
    struct ViewAimHit
    {
        float Distance{};

        // Range of the ray (the autoaim of the weapon, capped), 0 when it was not cast.
        float MaxDistance{};

        // World point of a hit, zero on a miss.
        Vec3 Point{};
        std::int16_t Material{};
        HitKind Kind{};
        std::uint32_t ObjectHandle{};
        ::Function Function{};
        std::uint8_t Subtype{};
        ViewRelation Relation{};

        // Row of the player whose biped was hit, 0xFF when it is not one.
        std::uint8_t PlayerRow{};
        std::int16_t RegionIndex{};
        bool IsHead{};
        std::uint8_t SurfaceFlags{};
    };

    // One ray of the perception pattern (F4.2). The direction comes from the pattern and FrameYaw, it is not stored.
    struct ViewRayHit
    {
        float Distance{};
        HitKind Kind{};
        std::uint32_t ObjectHandle{};

        // Global material of a static hit, -1 for none.
        std::int16_t Material{};
        ::Function Function{};
        ViewRelation Relation{};
        std::uint8_t SurfaceFlags{};
    };

    // What the agent knows of one row of Players (F4.3).
    struct ViewOther
    {
        ViewRelation Relation{};

        // Bits of ViewChannel.
        std::uint8_t Channels{};
        bool IsAlive{};
        Vec3 Position{};
        Vec3 Velocity{};
        float FacingYaw{};
        float Vitality{};
        float Shield{};
        std::uint8_t WeaponSubtype{};
        bool IsCrouching{};
        bool CarriesFlag{};

        // -1 below the agent, 0 level, 1 above. Radar only.
        std::int8_t Elevation{};
    };

    // One of the two flags (F4.3).
    struct ViewFlag
    {
        FlagState State{};
        Vec3 Position{};

        // Row of the carrier, 0xFF without one.
        std::uint8_t CarrierRow{};
        std::uint32_t ResetTicks{};
        bool IsResetRunning{};
        std::uint8_t AlliesInZone{};
        Vec3 StandPosition{};
    };

    // A sound the agent heard (F4.5).
    struct ViewSound
    {
        ViewSoundKind Kind{};
        Function SourceFunction{};
        std::uint8_t SourceSubtype{};
        Vec3 Direction{};
        float Intensity{};
        std::uint32_t AgeTicks{};
    };

    // An event the agent knows of (F4.6).
    struct ViewNotice
    {
        Event Kind{};
        std::uint8_t CauseRow{};
        std::uint8_t EffectRow{};
        ViewRelation CauseRelation{};
        ViewRelation EffectRelation{};
        ViewFlagSide FlagSide{};
        std::uint32_t AgeTicks{};
    };

    // Everything one agent can know in one tick. The sections after Self are filled by F4.2 to F4.6 and are zero until then.
    struct View
    {
        ViewHead Head{};
        ViewSelf Self{};
        ViewAimHit AimHit{};

        // Where the rays of the pattern start from (bounding center of the vehicle or of the biped), zero when they were not cast.
        Vec3 RayOrigin{};
        std::array<ViewRayHit, k_ViewRayCount> Rays{};
        std::array<ViewOther, k_MaxPlayers> Others{};
        std::array<ViewFlag, k_ViewFlags> Flags{};
        std::uint8_t SoundCount{};
        std::array<ViewSound, k_ViewSounds> Sounds{};
        std::uint8_t NoticeCount{};
        std::array<ViewNotice, k_ViewNotices> Notices{};
    };

    // One View per row of Players, in the same order. Rows past the players of the tick are zero.
    struct ViewTable : std::array<View, k_MaxPlayers>
    {
    };
}