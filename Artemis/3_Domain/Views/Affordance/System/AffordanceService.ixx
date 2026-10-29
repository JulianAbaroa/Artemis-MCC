export module Views.Affordance.System;

import Common.Math.Type;
import Common.Semantic.Type;
import Common.Log.System;
import Blam.Runtime.Object.Type;
import Blam.Runtime.Object.State;
import Blam.Runtime.Interaction.Type;
import Blam.Runtime.Interaction.State;
import Perception.Classifier.Type;
import Perception.Classifier.State;
import Perception.ObjectGraph.Type;
import Perception.ObjectGraph.State;
import Perception.Collidable.System;
import Views.Self.State;
import Views.Affordance.Type;
import Views.Affordance.State;
import std;

export namespace Views::Affordance::System
{
    class AffordanceService
    {
    private:
        using Vec3 = Common::Math::Type::Vec3;
        using AliveObject = Blam::Runtime::Object::Type::Alive::AliveObject;
        using ObjectTable = Blam::Runtime::Object::Type::Alive::ObjectTable;
        using AliveInteraction = Blam::Runtime::Interaction::Type::Alive::AliveInteraction;
        using Vehicle = Blam::Runtime::Object::Type::Vehicle::Vehicle;
        using SeatMarker = Blam::Runtime::Object::Type::Vehicle::Seat::SeatMarker;
        using BoneMatrixTable = Blam::Runtime::Object::Type::BoneMatrix::BoneMatrixTable;
        using ObjectTableStore = Blam::Runtime::Object::State::ObjectTableStore;
        using BoneMatricesStore = Blam::Runtime::Object::State::BoneMatricesStore;
        using InteractionStore = Blam::Runtime::Interaction::State::InteractionStore;
        using Function = Common::Semantic::Type::Function;
        using Classified = Perception::Classifier::Type::Classified;
        using ObjectNode = Perception::ObjectGraph::Type::ObjectNode;
        using ObjectNodes = Perception::ObjectGraph::Type::ObjectNodes;
        using ClassifierStore = Perception::Classifier::State::ClassifierStore;
        using ObjectGraphStore = Perception::ObjectGraph::State::ObjectGraphStore;
        using CollidableService = Perception::Collidable::System::CollidableService;
        using Affordance = Views::Affordance::Type::Affordance;
        using SeatStatus = Views::Affordance::Type::SeatStatus;
        using AffordanceBehavior = Views::Affordance::Type::AffordanceBehavior;
        using AffordanceActivation = Views::Affordance::Type::AffordanceActivation;

        using Log = Common::Log::System::Log;
        using SelfStore = Views::Self::State::SelfStore;
        using AffordanceStore = Views::Affordance::State::AffordanceStore;

    public:
        AffordanceService(Log& log, ObjectTableStore& objectStore,
            BoneMatricesStore& boneMatricesStore, InteractionStore& interactionStore,
            SelfStore& selfStore, ClassifierStore& classifierStore,
            ObjectGraphStore& objectGraphStore, AffordanceStore& affordanceStore, CollidableService& collidableService) :
            m_Log(log), m_ObjectStore(objectStore),
            m_BoneMatricesStore(boneMatricesStore),
            m_InteractionStore(interactionStore), m_SelfStore(selfStore),
            m_ClassifierStore(classifierStore), m_ObjectGraphStore(objectGraphStore),
            m_AffordanceStore(affordanceStore),
            m_CollidableService(collidableService)  {}
        ~AffordanceService() = default;

        auto Update() -> void;

        auto Cleanup() -> void;

    private:
        Log& m_Log;
        ObjectTableStore& m_ObjectStore;
        BoneMatricesStore& m_BoneMatricesStore;
        InteractionStore& m_InteractionStore;
        SelfStore& m_SelfStore;
        ClassifierStore& m_ClassifierStore;
        ObjectGraphStore& m_ObjectGraphStore;
        AffordanceStore& m_AffordanceStore;
        CollidableService& m_CollidableService;

        auto BuildVehicleAffordance(const AliveObject& object,
            const Vec3& selfPosition,
            std::uint32_t selfBipedHandle, const AliveInteraction& interaction,
            const ObjectNodes& objectGraph, const ObjectTable& objectTable, Affordance& out) const -> bool;

        auto BuildAffordance(const AliveObject& object, const Classified& classified,
            const Vec3& selfPosition, const AliveInteraction& interaction) const -> Affordance;

        auto ResolveSeatStatuses(const Vehicle& vehi,
            const AliveObject& object, const ObjectNodes& objectGraph,
            const ObjectTable& objectTable, const Vec3& selfPosition, std::uint32_t selfBipedHandle,
            std::vector<SeatStatus>& outSeats) const -> bool;

        auto CollectVehiclePartHandles(std::uint32_t vehicleHandle,
            const ObjectNodes& objectGraph, const ObjectTable& objectTable) const -> std::vector<std::uint32_t>;

        auto CollectBipedChildHandles(std::uint32_t vehicleHandle,
            const ObjectNodes& objectGraph, const ObjectTable& objectTable) const -> std::vector<std::uint32_t>;

        auto GetSeatWorldPosition(const BoneMatrixTable& bones, const SeatMarker& seatMarker,
            const AliveObject& vehicleObject) const -> std::optional<Vec3>;

        auto Distance(const float a[3], const float b[3]) const -> float;
        auto Distance(const Vec3& a, const Vec3& b) const -> float;

        auto DeriveBehaviors(Function function) const -> std::vector<AffordanceBehavior>;
        auto DeriveActivation(Function function) const -> AffordanceActivation;
    };
}