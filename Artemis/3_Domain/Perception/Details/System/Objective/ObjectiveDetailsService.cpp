module Perception.Details.System;
import :ObjectiveDetails;

import Common.Engine.Type;
import Common.Semantic.Type;
import Common.Team.Type;
import Common.ZoneShape.System;
import Blam.Layout.Game.Type;
import Blam.Runtime.Object.Type;
import Blam.Runtime.Game.Type;
import Perception.Classifier.Type;
import Perception.Entities.Type;
import Perception.Players.Type;

namespace
{
    namespace Subtype = Common::Semantic::Type::Subtype;
    namespace Zone = Common::ZoneShape::System;

    using Common::Semantic::Type::FlagState;
    using Common::Semantic::Type::Function;
    using Common::Team::Type::Team;
    using Blam::Runtime::Game::Type::Alive::AliveGame;
    using Blam::Runtime::Game::Type::Alive::AliveMegaloObject;
    using Blam::Runtime::Object::Type::Weapon::Weapon;
    using Perception::Classifier::Type::Classified;
    using Perception::Details::Type::ObjectiveDetail;
    using Perception::Details::Type::ObjectiveDetails;
    using Perception::Details::Type::StandDetail;
    using Perception::Details::Type::ZoneDetail;
    using Perception::Details::Type::ZoneDetails;
    using Perception::Entities::Type::Entities;
    using Perception::Entities::Type::Entity;
    using Perception::Players::Type::Player;
    using Perception::Players::Type::Players;

    using Common::Engine::Type::k_MaxTeams;
    using Common::Engine::Type::k_NoHandle;
    using Blam::Layout::Game::Type::Offset::IsTimerRunning;
    using Blam::Layout::Game::Type::Offset::TimerTicksCeil;

    // State of the Megalo element of a flag: 0 home, 1 carried, 2 dropped.
    constexpr std::uint32_t k_MegaloHome{ 0 };
    constexpr std::uint32_t k_MegaloDropped{ 2 };

    // Distance, horizontal and vertical, within which a flag without a usable Megalo state counts as standing on its stand (world units).
    constexpr float k_HomeRadius{ 1.0f };

    // return: Whether the flag is at its stand: within k_HomeRadius of the stand horizontally and vertically. False without a stand.
    // note: The engine recreates a dropped flag as a new object that has no Megalo element for about ten ticks. A flag at its stand
    // is home, anywhere else it lies on the ground.
    auto IsAtStand(const Entity& flag, const Entity* stand) -> bool
    {
        if (stand == nullptr) return false;

        const float horizontal{ std::hypot(flag.WorldOrigin.X - stand->WorldOrigin.X, flag.WorldOrigin.Y - stand->WorldOrigin.Y) };
        const float vertical{ std::abs(flag.WorldOrigin.Z - stand->WorldOrigin.Z) };
        return horizontal <= k_HomeRadius && vertical <= k_HomeRadius;
    }

    // return: The team that plays the side of a base team in this round, Neutral when the side is not in use.
    auto OwnerOf(Team baseTeam, const AliveGame* game) -> Team
    {
        const std::size_t side{ static_cast<std::size_t>(baseTeam) };
        if (game == nullptr || side >= k_MaxTeams) return Team::Neutral;

        const std::uint8_t team{ game->TeamMapping[side] };
        return team < k_MaxTeams ? static_cast<Team>(team) : Team::Neutral;
    }
}

namespace
{
    // return: Live bipeds of players of the team inside the zone, 0 without the zone detail, the zone entity or the players.
    auto CountAllies(Team team, std::uint32_t zoneHandle, const Entities& entities, const ZoneDetails* zones, const Players* players) -> std::uint8_t
    {
        if (team == Team::Neutral || zones == nullptr || players == nullptr) return 0;

        const auto zoneIt{ zones->find(zoneHandle) };
        const Entity* zoneEntity{ entities.Find(zoneHandle) };
        if (zoneIt == zones->end() || zoneEntity == nullptr) return 0;

        std::uint8_t allies{ 0 };
        for (const Player& player : players->Rows)
        {
            if (player.Team != team || !player.IsAlive()) continue;

            const Entity* biped{ entities.Find(player.BipedHandle) };
            if (biped == nullptr) continue;

            if (Zone::Contains(zoneIt->second.Shape, zoneEntity->WorldOrigin, zoneEntity->Forward, zoneEntity->Up, biped->WorldOrigin)) ++allies;
        }

        return allies;
    }
}

namespace Perception::Details::System
{
    auto ObjectiveDetailsService::Update() -> void
    {
        const auto classifiedsPtr{ m_ClassifierStore.Acquire() };
        const auto objectTablePtr{ m_ObjectStore.Acquire() };
        const auto entitiesPtr{ m_EntitiesStore.Acquire() };
        if (!classifiedsPtr || !objectTablePtr || !entitiesPtr) return;

        // note: Before the first publish of these stores there is no owner, no Megalo state, no carrier and no zone.
        const auto gamePtr{ m_GameStore.Acquire() };
        const auto playersPtr{ m_PlayersStore.Acquire() };
        const auto zonesPtr{ m_ZoneDetailsStore.Acquire() };
        const AliveGame* game{ gamePtr.get() };

        std::unordered_map<std::uint32_t, const AliveMegaloObject*> megalo{};
        if (game != nullptr)
        {
            megalo.reserve(game->MegaloObjects.size());
            for (const AliveMegaloObject& element : game->MegaloObjects) megalo.emplace(element.ObjectHandle, &element);
        }

        std::unordered_map<std::uint32_t, const Player*> playerOfBiped{};
        if (playersPtr)
        {
            for (const Player& player : playersPtr->Rows)
            {
                if (player.BipedHandle != k_NoHandle) playerOfBiped.emplace(player.BipedHandle, &player);
            }
        }

        // Zone of each flag: the ObjectiveZone bound to it.
        std::unordered_map<std::uint32_t, std::uint32_t> zoneOfFlag{};
        for (const Classified& classified : *classifiedsPtr)
        {
            if (classified.Function == Function::ObjectiveZone && classified.BoundTo != k_NoHandle) zoneOfFlag.emplace(classified.BoundTo, classified.Handle);
        }

        ObjectiveDetails details{};

        for (const Classified& classified : *classifiedsPtr)
        {
            if (classified.Function == Function::ObjectiveSpawn && classified.Subtype == static_cast<std::uint8_t>(Subtype::ObjectiveSpawn::FlagStand))
            {
                const Entity* entity{ entitiesPtr->Find(classified.Handle) };
                if (entity == nullptr) continue;

                details.Stands.emplace(classified.Handle, StandDetail{ .Handle = classified.Handle, .BaseTeam = entity->Team, .OwnerTeam = OwnerOf(entity->Team, game) });
            }
        }

        for (const Classified& classified : *classifiedsPtr)
        {
            if (classified.Function != Function::ObjectiveItem || classified.Subtype != static_cast<std::uint8_t>(Subtype::ObjectiveItem::Flag)) continue;

            const auto objectIt{ objectTablePtr->find(classified.Handle) };
            const Entity* entity{ entitiesPtr->Find(classified.Handle) };
            if (objectIt == objectTablePtr->end() || entity == nullptr) continue;

            const Weapon* weapon{ std::get_if<Weapon>(&objectIt->second.Specific) };
            if (weapon == nullptr) continue;

            ObjectiveDetail flag{ .Handle = classified.Handle, .BaseTeam = entity->Team, .OwnerTeam = OwnerOf(entity->Team, game) };

            // The stand with the same base team. The lowest handle wins so the result does not depend on the container.
            for (const auto& [standHandle, stand] : details.Stands)
            {
                if (stand.BaseTeam == flag.BaseTeam && (flag.HomeStand == k_NoHandle || standHandle < flag.HomeStand)) flag.HomeStand = standHandle;
            }

            const auto megaloIt{ megalo.find(classified.Handle) };
            const AliveMegaloObject* element{ megaloIt == megalo.end() ? nullptr : megaloIt->second };

            if (weapon->HolderUnit != k_NoHandle)
            {
                flag.State = FlagState::Carried;
                flag.CarrierBiped = weapon->HolderUnit;

                const auto playerIt{ playerOfBiped.find(weapon->HolderUnit) };
                if (playerIt != playerOfBiped.end()) flag.CarrierPlayer = playerIt->second->Handle;
            }
            else if (element != nullptr && element->State == k_MegaloHome)
            {
                flag.State = FlagState::Home;
            }
            else if (element != nullptr && element->State == k_MegaloDropped)
            {
                flag.State = FlagState::Dropped;
            }
            else
            {
                // note: No Megalo element yet, or one that still says carried while nobody holds the flag (it lags the drop by up to
                // ten ticks): the position tells. Nothing is remembered from the tick before.
                flag.State = IsAtStand(*entity, entitiesPtr->Find(flag.HomeStand)) ? FlagState::Home : FlagState::Dropped;
            }

            if (flag.State == FlagState::Dropped && element != nullptr && element->State == k_MegaloDropped)
            {
                flag.ResetTicks = TimerTicksCeil(element->ResetTimer);
                flag.IsResetRunning = IsTimerRunning(element->ResetTimer);
            }

            const auto zoneIt{ zoneOfFlag.find(classified.Handle) };
            if (zoneIt != zoneOfFlag.end()) flag.AlliesInZone = CountAllies(flag.OwnerTeam, zoneIt->second, *entitiesPtr, zonesPtr.get(), playersPtr.get());
            details.Flags.emplace(classified.Handle, flag);
        }

        m_ObjectiveDetailsStore.Publish(std::move(details));
    }

    auto ObjectiveDetailsService::Cleanup() -> void
    {
        m_ObjectiveDetailsStore.Cleanup();

        m_Log.Message("[ObjectiveDetailsService] INFO: Cleanup completed.");
    }
}