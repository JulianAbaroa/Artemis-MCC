module Perception.Flags.System;

import Common.Team.Type;
import Blam.Runtime.Game.Type;
import Blam.Runtime.Object.Type;
import Perception.Details.System;

namespace
{
    using Common::Team::Type::Team;
    using Blam::Runtime::Game::Type::Alive::AliveMegaloObject;
    using Blam::Runtime::Object::Type::Alive::AliveObject;
    using Blam::Runtime::Object::Type::Weapon::Weapon;
    using Perception::Details::System::ObjectiveDetailsService;
    using Perception::Details::Type::ObjectiveDetail;
    using Perception::Details::Type::ObjectiveDetails;
}

namespace Perception::Flags::System
{
    auto FlagPassService::BuildFlags() -> ObjectiveDetails
    {
        ObjectiveDetails flags{};

        const auto objectsPtr{ m_LightObjectStore.Acquire() };
        const auto gamePtr{ m_LightGameStore.Acquire() };
        const auto lastPtr{ m_ObjectiveDetailsStore.Acquire() };
        if (!objectsPtr || !gamePtr || !lastPtr) return flags;

        // The weapons whose tag is a flag.
        std::vector<std::pair<std::uint32_t, const AliveObject*>> candidates{};
        for (const auto& [handle, object] : *objectsPtr)
        {
            if (!std::holds_alternative<Weapon>(object.Specific)) continue;

            const auto* classification{ m_TagClassificationsStore.Find(object.FourCC, object.TagName) };
            if (classification != nullptr && classification->IsFlag()) candidates.emplace_back(handle, &object);
        }
        if (candidates.empty()) return flags;

        std::unordered_map<std::uint32_t, const AliveMegaloObject*> megalo{};
        megalo.reserve(gamePtr->MegaloObjects.size());
        for (const AliveMegaloObject& element : gamePtr->MegaloObjects) megalo.emplace(element.ObjectHandle, &element);

        const auto playersPtr{ m_PlayersStore.Acquire() };
        const auto playerOfBiped{ ObjectiveDetailsService::MapPlayersOfBipeds(playersPtr.get()) };

        for (const auto& [handle, objectPtr] : candidates)
        {
            const AliveObject& object{ *objectPtr };
            const Weapon& weapon{ std::get<Weapon>(object.Specific) };

            const auto megaloIt{ megalo.find(handle) };
            const AliveMegaloObject* element{ megaloIt == megalo.end() ? nullptr : megaloIt->second };

            const Team baseTeam{ object.Multiplayer ? object.Multiplayer->Team : Team::Neutral };
            ObjectiveDetail flag{ ObjectiveDetailsService::MakeFlag(handle, baseTeam, object.WorldOrigin, weapon, element, gamePtr.get(), *lastPtr,
                playerOfBiped) };

            for (const auto& [lastHandle, last] : lastPtr->Flags)
            {
                if (last.BaseTeam == baseTeam) flag.AlliesInZone = last.AlliesInZone;
            }

            flags.Flags.emplace(handle, flag);
        }

        return flags;
    }

    auto FlagPassService::Update() -> void
    {
        const auto gamePtr{ m_LightGameStore.Acquire() };
        if (!gamePtr || !m_ObjectiveDetailsStore.Acquire()) return;

        m_EventsService.UpdateFlags(*gamePtr, this->BuildFlags());
    }
}