export module Perception.Details.Type:Weapon;

import Common.Engine.Type;
import Common.Semantic.Type;
import Blam.Runtime.Object.Type;
import std;

namespace
{
    using Common::Engine::Type::k_NoHandle;
    using Blam::Runtime::Object::Type::Weapon::WeaponMagazine;
}

export namespace Perception::Details::Type
{
    // State of one live weapon, in a hand, on the floor or mounted (D254).
    // note: The pose and the holder are in Entities. The fields that come from the tag keep their neutral value when the weapon has no
    // definition: MagazineCount 0, IsEnergyWeapon false, ChargeFraction NaN.
    struct WeaponDetail
    {
        std::uint32_t Handle{};

        // Magazines of the tag that hold rounds, 0 to 2. Only those are meaningful in Magazines, in the order of the tag.
        std::uint8_t MagazineCount{};
        std::array<WeaponMagazine, 2> Magazines{};

        // The tag has no magazine with rounds: the weapon spends energy.
        bool IsEnergyWeapon{ false };

        // Energy left from 0 to 1 on an energy weapon, NaN on the others.
        float Energy{ std::numeric_limits<float>::quiet_NaN() };

        // Heat from 0 to 1.
        float Heat{};

        // The engine keeps the weapon overheated, venting and then cooling, until it can fire again. The heat alone does not tell.
        bool IsOverheated{ false };

        // A reload is in progress.
        bool IsReloading{ false };

        // Stage of the charged fire mode (trigger 0).
        Common::Semantic::Type::ChargeState ChargeState{ Common::Semantic::Type::ChargeState::None };

        // Progress of the charge from 0 to 1, NaN when no trigger of the tag charges.
        // note: Charging counts the ticks since the first one seen in that state, over the charging time of the tag. A weapon seen
        // already charging (after a load) starts from there.
        float ChargeFraction{ std::numeric_limits<float>::quiet_NaN() };

        // Object the weapon tracks of any kind, k_NoHandle when it tracks nothing.
        std::uint32_t TrackedObject{ k_NoHandle };

        // The tracked object is locked.
        bool IsLocked{ false };

        // The weapon would fire now: not busy, not overheated and with energy or a loaded round.
        // note: Does not cover the trigger being held, the holder being able to fire (dead, in a menu) nor a melee in progress.
        bool CanFireNow{ false };
    };

    struct WeaponDetails : std::unordered_map<std::uint32_t, WeaponDetail>
    {
        using unordered_map::unordered_map;
    };

    // First engine tick each weapon was seen in the Charging state, by handle. Only the weapons that are charging are in it.
    struct WeaponChargeMemory : std::unordered_map<std::uint32_t, std::uint32_t>
    {
        using unordered_map::unordered_map;
    };
}