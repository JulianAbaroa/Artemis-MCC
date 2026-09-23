export module Egocentric.Affordance.System;

import Service.Logs.System;
import Common.Math.Type;
import Tables.Object.Type;
import Tables.Object.State;
import Tables.Interaction.Type;
import Tables.Interaction.State;
import Relations.Classifier.Type;
import Relations.Classifier.State;
import Relations.ObjectGraph.Type;
import Relations.ObjectGraph.State;
import Relations.PlayerGraph.Type;
import Relations.PlayerGraph.State;
import Environment.Collidable.System;
import Egocentric.Self.State;
import Egocentric.Affordance.Type;
import Egocentric.Affordance.State;
import std;

export namespace Egocentric::Affordance::System
{
    class AffordanceService
    {
    private:
        using Vec3 = Common::Math::Type::Vec3;
        using AliveObject = Tables::Object::Type::Alive::Object;
        using ObjectTable = std::unordered_map<std::uint32_t, AliveObject>;
        using AliveInteraction = Tables::Interaction::Type::Alive::Interaction;
        using Vehicle = Tables::Object::Type::Vehicle::Vehicle;
        using SeatMarker = Tables::Object::Type::Vehicle::Seat::Marker;
        using BoneMatrixTable = Tables::Object::Type::BoneMatrix::BoneMatrixTable;
        using ObjectTableStore = Tables::Object::State::ObjectTableStore;
        using BoneMatricesStore = Tables::Object::State::BoneMatricesStore;
        using InteractionStore = Tables::Interaction::State::InteractionStore;
        using Role = Relations::Classifier::Type::Role;
        using ObjectNode = Relations::ObjectGraph::Type::ObjectNode;
        using ObjectGraph = std::unordered_map<std::uint32_t, ObjectNode>;
        using PlayerTree = Relations::PlayerGraph::Type::PlayerTree;
        using PlayerGraph = std::vector<PlayerTree>;
        using ClassifierStore = Relations::Classifier::State::ClassifierStore;
        using ObjectGraphStore = Relations::ObjectGraph::State::ObjectGraphStore;
        using PlayerGraphStore = Relations::PlayerGraph::State::PlayerGraphStore;
        using CollidableService = Environment::Collidable::System::CollidableService;
        using Affordance = Egocentric::Affordance::Type::Affordance;
        using SeatStatus = Egocentric::Affordance::Type::SeatStatus;
        using AffordanceBehavior = Egocentric::Affordance::Type::Behavior;
        using AffordanceActivation = Egocentric::Affordance::Type::Activation;

        using SelfStore = Egocentric::Self::State::SelfStore;
        using AffordanceStore = Egocentric::Affordance::State::AffordanceStore;
        using LogsService = Service::Logs::System::LogsService;

    public:
        AffordanceService(LogsService& logsService, ObjectTableStore& objectStore, 
            BoneMatricesStore& boneMatricesStore, InteractionStore& interactionStore, 
            SelfStore& selfStore, ClassifierStore& classifierStore, 
            ObjectGraphStore& objectGraphStore, PlayerGraphStore& playerGraphStore, 
            AffordanceStore& affordanceStore, CollidableService& collidableService) : 
            m_LogsService(logsService), m_ObjectStore(objectStore), 
            m_BoneMatricesStore(boneMatricesStore),
            m_InteractionStore(interactionStore), m_SelfStore(selfStore), 
            m_ClassifierStore(classifierStore), m_ObjectGraphStore(objectGraphStore), 
            m_PlayerGraphStore(playerGraphStore), m_AffordanceStore(affordanceStore), 
            m_CollidableService(collidableService)  {}
        ~AffordanceService() = default;

        auto Update() -> void;

        auto Cleanup() -> void;

    private:
        LogsService& m_LogsService;
        ObjectTableStore& m_ObjectStore;
        BoneMatricesStore& m_BoneMatricesStore;
        InteractionStore& m_InteractionStore;
        SelfStore& m_SelfStore;
        ClassifierStore& m_ClassifierStore;
        ObjectGraphStore& m_ObjectGraphStore;
        PlayerGraphStore& m_PlayerGraphStore;
        AffordanceStore& m_AffordanceStore;
        CollidableService& m_CollidableService;

        auto BuildVehicleAffordance(const AliveObject& object,
            const Vec3& selfPosition,
            std::uint32_t selfBipedHandle, const AliveInteraction& interaction,
            const ObjectGraph& objectGraph, const PlayerGraph& playerGraph,
            const ObjectTable& objectTable, Affordance& out) const -> bool;

        auto BuildAffordance(const AliveObject& object, Role role, 
            const Vec3& selfPosition, const AliveInteraction& interaction) const -> Affordance;

        auto ResolveSeatStatuses(const Vehicle& vehi,
            const AliveObject& object, const ObjectGraph& objectGraph,
            const ObjectTable& objectTable, const PlayerGraph& playerGraph,
            const Vec3& selfPosition, std::uint32_t selfBipedHandle,
            std::vector<SeatStatus>& outSeats) const -> bool;

        auto CollectVehiclePartHandles(std::uint32_t vehicleHandle,
            const ObjectGraph& objectGraph, const ObjectTable& objectTable) const -> std::vector<std::uint32_t>;

        auto CollectBipedChildHandles(std::uint32_t vehicleHandle,
            const ObjectGraph& objectGraph, const ObjectTable& objectTable) const -> std::vector<std::uint32_t>;

        auto GetSeatWorldPosition(const BoneMatrixTable& bones, const SeatMarker& seatMarker, 
            const AliveObject& vehicleObject) const -> std::optional<Vec3>;

        auto Distance(const float a[3], const float b[3]) const -> float;
        auto Distance(const Vec3& a, const Vec3& b) const -> float;

        auto DeriveBehaviors(Role role) const -> std::vector<AffordanceBehavior>;
        auto DeriveActivation(Role role) const -> AffordanceActivation;
    };
}