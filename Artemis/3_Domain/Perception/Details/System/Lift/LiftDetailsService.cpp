module Perception.Details.System;
import :LiftDetails;

import Common.Engine.Type;
import Common.Math.Type;
import Common.Math.System;
import Common.Semantic.Type;
import Blam.MapModel.Definitions.Type;
import Blam.Runtime.Object.Type;
import Perception.Classifier.Type;

namespace
{
    using Common::Engine::Type::k_NoHandle;
    using Common::Math::Type::Vec3;
    using Common::Math::System::Add;
    using Common::Math::System::Cross;
    using Common::Math::System::Distance;
    using Common::Math::System::Rotate;
    using Common::Math::System::Scale;
    using Common::Semantic::Type::Function;
    using Blam::MapModel::Definitions::Type::Hlmt::Hlmt;
    using Blam::MapModel::Definitions::Type::Mode::Mode;
    using Blam::MapModel::Definitions::Type::Mode::Marker;
    using Blam::MapModel::Definitions::Type::Object::ObjectKind;
    using Blam::MapModel::Definitions::Type::Phmo::PhantomType;
    using Blam::MapModel::Definitions::Type::Phmo::Phmo;
    using Blam::MapModel::Definitions::State::DefinitionsStore;
    using Blam::Runtime::Object::Type::Alive::AliveObject;
    using Perception::Classifier::Type::Classified;
    using Perception::Details::Type::LiftAffects;
    using Perception::Details::Type::LiftBipedEntry;
    using Perception::Details::Type::LiftDetail;
    using Perception::Details::Type::LiftDetails;
    using Perception::Details::Type::LiftExit;
    using Perception::Details::Type::LiftMemory;
    using Perception::Details::Type::LiftStage;

    using Blam::MapModel::Definitions::Type::Phmo::k_PhantomIgnoresPlayers;
    using Blam::MapModel::Definitions::Type::Phmo::k_PhantomIgnoresNonPlayers;
    using Blam::MapModel::Definitions::Type::Phmo::k_PhantomIgnoresBipeds;
    using Blam::MapModel::Definitions::Type::Phmo::k_PhantomIgnoresVehicles;
    using Blam::MapModel::Definitions::Type::Phmo::k_PhantomIgnoresWeapons;
    using Blam::MapModel::Definitions::Type::Phmo::k_PhantomIgnoresEquipment;
    using Blam::MapModel::Definitions::Type::Phmo::k_PhantomIgnoresCrates;

    // Furthest a biped can be from a lift when it enters the phantom volume and still be assigned to it, world units.
    // note: Measured 0.34 to 0.86 in the 11 lifts of forge_halo (A12). The doors and other phantoms are farther.
    constexpr float k_LiftOwnerRadius{ 3.0f };

    // return: Whether the phantom type ignores none of the kinds of the mask.
    auto Pushes(std::uint32_t flags, std::uint32_t ignoredMask) -> bool
    {
        return (flags & ignoredMask) == 0;
    }

    // return: The LiftAffects bits of the flags of a phantom type.
    auto AffectsOf(std::uint32_t flags) -> std::uint8_t
    {
        std::uint8_t affects{ 0 };
        if (Pushes(flags, k_PhantomIgnoresPlayers | k_PhantomIgnoresBipeds)) affects |= static_cast<std::uint8_t>(LiftAffects::Players);
        if (Pushes(flags, k_PhantomIgnoresNonPlayers | k_PhantomIgnoresBipeds)) affects |= static_cast<std::uint8_t>(LiftAffects::Bipeds);
        if (Pushes(flags, k_PhantomIgnoresVehicles)) affects |= static_cast<std::uint8_t>(LiftAffects::Vehicles);
        if (Pushes(flags, k_PhantomIgnoresWeapons) || Pushes(flags, k_PhantomIgnoresEquipment) || Pushes(flags, k_PhantomIgnoresCrates))
        {
            affects |= static_cast<std::uint8_t>(LiftAffects::Items);
        }
        return affects;
    }

    // return: The first marker of the group of the render model with the name, null when the name is empty or has no group or marker.
    auto FindMarker(const Mode* mode, std::uint32_t nameId) -> const Marker*
    {
        if (mode == nullptr || nameId == 0) return nullptr;

        for (const auto& group : mode->MarkerGroups)
        {
            if (group.NameId == nameId && !group.Markers.empty()) return &group.Markers.front();
        }

        return nullptr;
    }

    // return: A vector of the frame of the object (Forward, Left, Up) in world space.
    // note: Left is Up x Forward. The lifts are fixed objects, so the node 0 frame is the frame of the object and the node of the marker
    // is not applied.
    auto ToWorld(const Vec3& local, const AliveObject& object) -> Vec3
    {
        const Vec3 left{ Cross(object.Up, object.Forward) };
        return Add(Add(Scale(object.Forward, local.X), Scale(left, local.Y)), Scale(object.Up, local.Z));
    }

    // return: The stage of the phantom type: the marker and the push, in world space.
    auto MakeStage(const PhantomType& type, const Mode* mode, const AliveObject& object) -> LiftStage
    {
        LiftStage stage{ .Position = object.WorldOrigin, .Direction = object.Up, .Acceleration = type.DirectionAcceleration,
            .MaximumVelocity = type.DirectionMaximumVelocity, .AxisAcceleration = type.AxisAcceleration,
            .AxisMaximumVelocity = type.AxisMaximumVelocity, .Affects = AffectsOf(type.Flags) };

        if (const Marker* marker{ FindMarker(mode, type.MarkerName) })
        {
            stage.Position = Add(object.WorldOrigin, ToWorld(marker->Translation, object));
            stage.Direction = ToWorld(Rotate(marker->Rotation, Vec3{ 1.0f, 0.0f, 0.0f }), object);
        }

        return stage;
    }

    // return: The stages of the physics model of the tag of the lift: one per phantom type that pushes players or bipeds, in tag order.
    auto StagesOf(const DefinitionsStore& definitions, const AliveObject& object) -> std::vector<LiftStage>
    {
        std::vector<LiftStage> stages{};

        const Hlmt* hlmt{ definitions.m_Hlmt.Get(definitions.ModelOf(object.TagName)) };
        if (hlmt == nullptr) return stages;

        const Phmo* phmo{ definitions.m_Phmo.Get(hlmt->PhysicsModelTagName) };
        if (phmo == nullptr) return stages;

        const Mode* mode{ definitions.m_Mode.Get(hlmt->RenderModelTagName) };
        for (const PhantomType& type : phmo->PhantomTypes)
        {
            const std::uint8_t affects{ AffectsOf(type.Flags) };
            if ((affects & (static_cast<std::uint8_t>(LiftAffects::Players) | static_cast<std::uint8_t>(LiftAffects::Bipeds))) == 0) continue;

            stages.push_back(MakeStage(type, mode, object));
        }

        return stages;
    }

    // return: The lift whose origin is nearest to the position and within k_LiftOwnerRadius, k_NoHandle when there is none.
    // note: The lowest handle wins a tie, so the result does not depend on the order of the table.
    auto NearestLift(const LiftDetails& lifts, const std::unordered_map<std::uint32_t, Vec3>& origins, const Vec3& position) -> std::uint32_t
    {
        std::uint32_t nearest{ k_NoHandle };
        float nearestDistance{ k_LiftOwnerRadius };

        for (const auto& [handle, detail] : lifts)
        {
            const float distance{ Distance(origins.at(handle), position) };
            if (distance > nearestDistance || (distance == nearestDistance && handle > nearest)) continue;

            nearest = handle;
            nearestDistance = distance;
        }

        return nearest;
    }
}

namespace Perception::Details::System
{
    auto LiftDetailsService::Update() -> void
    {
        auto classifiedsPtr = m_ClassifierStore.Acquire();
        auto objectTablePtr = m_ObjectStore.Acquire();
        if (!classifiedsPtr || !objectTablePtr) return;

        const auto game{ m_GameStore.Acquire() };
        const std::uint32_t tick{ game ? game->EngineTick : 0 };

        const bool hasDefinitions{ m_DefinitionsStore.m_Hlmt.IsFrozen() && m_DefinitionsStore.m_Phmo.IsFrozen() && m_DefinitionsStore.m_Mode.IsFrozen() };
        LiftDetails lifts{};
        std::unordered_map<std::uint32_t, Vec3> origins{};

        for (const Classified& classified : *classifiedsPtr)
        {
            if (classified.Function != Function::Lift) continue;

            const auto objectIt{ objectTablePtr->find(classified.Handle) };
            if (objectIt == objectTablePtr->end()) continue;

            LiftDetail detail{ .Handle = classified.Handle };
            if (hasDefinitions) detail.Stages = StagesOf(m_DefinitionsStore, objectIt->second);

            lifts.emplace(classified.Handle, std::move(detail));
            origins.emplace(classified.Handle, objectIt->second.WorldOrigin);
        }

        // The bipeds in the order of their handle, so two exits of the same tick always leave the same last one.
        std::vector<std::uint32_t> bipeds{};
        for (const auto& [handle, object] : *objectTablePtr)
        {
            if (object.Profile.ObjectKind == ObjectKind::Biped) bipeds.push_back(handle);
        }
        std::ranges::sort(bipeds);

        LiftMemory& memory{ m_LiftMemoryStore.Get() };
        LiftMemory next{};

        // A lift that is gone loses its exit.
        for (const auto& [handle, exit] : memory.Exits)
        {
            if (lifts.contains(handle)) next.Exits.emplace(handle, exit);
        }

        for (const std::uint32_t handle : bipeds)
        {
            const AliveObject& biped{ objectTablePtr->at(handle) };
            const auto before{ memory.Bipeds.find(handle) };
            const bool wasInside{ before != memory.Bipeds.end() };

            if (biped.IsInPhantom)
            {
                next.Bipeds.emplace(handle, wasInside ? before->second
                    : LiftBipedEntry{ .Lift = NearestLift(lifts, origins, biped.WorldOrigin) });
            }
            else if (wasInside && lifts.contains(before->second.Lift))
            {
                next.Exits[before->second.Lift] = LiftExit{ .Tick = tick, .Velocity = biped.LinearVelocity, .Biped = handle };
            }
        }

        for (auto& [handle, detail] : lifts)
        {
            const auto exitIt{ next.Exits.find(handle) };
            if (exitIt == next.Exits.end()) continue;

            detail.HasExit = true;
            detail.ExitTick = exitIt->second.Tick;
            detail.ExitVelocity = exitIt->second.Velocity;
            detail.ExitBiped = exitIt->second.Biped;
        }

        memory = std::move(next);
        m_LiftDetailsStore.Publish(std::move(lifts));
    }

    auto LiftDetailsService::Cleanup() -> void
    {
        m_LiftMemoryStore.Cleanup();
        m_LiftDetailsStore.Cleanup();

        m_Log.Message("[LiftDetailsService] INFO: Cleanup completed.");
    }
}