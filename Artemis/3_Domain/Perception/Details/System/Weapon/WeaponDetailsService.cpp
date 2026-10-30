module Perception.Details.System;
import :WeaponDetails;

import Common.Engine.Type;
import Common.Semantic.Type;
import Blam.MapModel.Definitions.Type;
import Blam.Runtime.Object.Type;
import std;

namespace
{
    using Common::Engine::Type::k_NoHandle;
    using Common::Engine::Type::k_TicksPerSecond;
    using Common::Semantic::Type::ChargeState;
    using Blam::MapModel::Definitions::Type::Weap::Weap;
    using Blam::MapModel::Definitions::Type::Weap::k_TriggerActionCharge;
    using Blam::Runtime::Object::Type::Weapon::Weapon;
    using Perception::Details::Type::WeaponChargeMemory;
    using Perception::Details::Type::WeaponDetail;
    using Perception::Details::Type::WeaponDetails;

    constexpr std::uint32_t k_LockedTrackingState{ 2 };
    constexpr std::size_t k_MaxMagazines{ 2 };

    // return: Seconds the tag takes to charge the weapon, or a negative number when no trigger charges.
    auto ChargingTimeOf(const Weap& weap) -> float
    {
        for (const auto& trigger : weap.Triggers)
        {
            if (trigger.PrimaryAction == k_TriggerActionCharge || trigger.SecondaryAction == k_TriggerActionCharge) return trigger.ChargingTime;
        }

        return -1.0f;
    }

    // return: Progress of the charge from 0 to 1, NaN when the weapon does not charge.
    auto ChargeFractionOf(ChargeState state, float chargingTime, std::uint32_t ticksCharging) -> float
    {
        if (chargingTime < 0.0f) return std::numeric_limits<float>::quiet_NaN();

        switch (state)
        {
        case ChargeState::Charged: return 1.0f;
        case ChargeState::Charging:
        {
            const float ticksToCharge{ chargingTime * k_TicksPerSecond };
            return ticksToCharge <= 0.0f ? 1.0f : (std::min)(1.0f, static_cast<float>(ticksCharging) / ticksToCharge);
        }
        default: return 0.0f;
        }
    }
}

namespace Perception::Details::System
{
    auto WeaponDetailsService::Update() -> void
    {
        auto objectTablePtr = m_ObjectStore.Acquire();
        if (!objectTablePtr) return;

        const auto game{ m_GameStore.Acquire() };
        const std::uint32_t tick{ game ? game->EngineTick : 0 };

        const bool hasDefinitions{ m_DefinitionsStore.m_Weap.IsFrozen() };
        WeaponChargeMemory& memory{ m_ChargeMemoryStore.Get() };
        WeaponChargeMemory charging{};
        WeaponDetails weapons{};

        for (const auto& [handle, object] : *objectTablePtr)
        {
            const Weapon* weapon{ std::get_if<Weapon>(&object.Specific) };
            if (weapon == nullptr) continue;

            const Weap* weap{ hasDefinitions ? m_DefinitionsStore.m_Weap.Get(object.TagName) : nullptr };

            WeaponDetail detail{ .Handle = handle };
            // note: A magazine counts only if it holds rounds. The plasma rifle and repeater tags carry one magazine of 0 rounds (their datum
            // magazine stays 0/0 while Total Energy rises with each shot), so they are energy weapons. The magazines that count fill
            // Magazines in order.
            if (weap != nullptr)
            {
                for (std::size_t index{ 0 }; index < (std::min)(weap->Magazines.size(), k_MaxMagazines); ++index)
                {
                    if (weap->Magazines[index].RoundsLoadedMaximum <= 0) continue;

                    detail.Magazines[detail.MagazineCount] = weapon->Magazines[index];
                    ++detail.MagazineCount;
                }
            }

            detail.IsEnergyWeapon = weap != nullptr && detail.MagazineCount == 0;
            if (detail.IsEnergyWeapon) detail.Energy = weapon->TotalEnergy;

            detail.Heat = weapon->TotalHeat;
            detail.IsOverheated = weapon->IsOverheated != 0;

            const bool isBusy{ weapon->BusyTimer != 0 };
            detail.IsReloading = weapon->Magazine0ReloadState != 0 && isBusy;

            detail.ChargeState = weapon->ChargeState;
            std::uint32_t chargeStart{ tick };
            if (weapon->ChargeState == ChargeState::Charging)
            {
                const auto previous{ memory.find(handle) };
                if (previous != memory.end() && previous->second <= tick) chargeStart = previous->second;
                charging.emplace(handle, chargeStart);
            }

            const float chargingTime{ weap ? ChargingTimeOf(*weap) : -1.0f };
            detail.ChargeFraction = ChargeFractionOf(weapon->ChargeState, chargingTime, tick - chargeStart);

            detail.TrackedObject = weapon->IsTracking != 0 ? weapon->TrackedObject : k_NoHandle;
            detail.IsLocked = weapon->IsTracking != 0 && weapon->TrackingState == k_LockedTrackingState;

            const bool hasAmmo{ detail.IsEnergyWeapon ? detail.Energy > 0.0f : weapon->Magazines[0].Loaded > 0 };
            detail.CanFireNow = !isBusy && !detail.IsOverheated && hasAmmo;

            weapons.emplace(handle, detail);
        }

        // note: Only the weapons still charging keep their start. The others, and those that left the table, are forgotten.
        memory = std::move(charging);
        m_WeaponDetailsStore.Publish(std::move(weapons));
    }

    auto WeaponDetailsService::Cleanup() -> void
    {
        m_ChargeMemoryStore.Cleanup();
        m_WeaponDetailsStore.Cleanup();

        m_Log.Message("[WeaponDetailsService] INFO: Cleanup completed.");
    }
}