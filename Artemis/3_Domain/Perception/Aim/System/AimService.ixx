export module Perception.Aim.System;

import Common.Math.Type;
import Common.Log.System;
import Blam.MapModel.World.Type;
import Blam.MapModel.Vitality.Type;
import Blam.MapModel.Vitality.State;
import Blam.Runtime.Object.Type;
import Blam.Runtime.Object.State;
import Perception.Aim.Type;
import Perception.Aim.State;
import std;

export namespace Perception::Aim::System
{
    class AimService
    {
    private:
        using Vec3 = Common::Math::Type::Vec3;
        using AliveObject = Blam::Runtime::Object::Type::Alive::AliveObject;
        using ObjectTable = Blam::Runtime::Object::Type::Alive::ObjectTable;
        using BoneMatrixTable = Blam::Runtime::Object::Type::BoneMatrix::BoneMatrixTable;
        using Vitality = Blam::MapModel::Vitality::Type::Vitality::Vitality;
        using ModelLinkAnchor = Blam::MapModel::World::Type::ModelLink::ModelLinkAnchor;
        using Aim = Perception::Aim::Type::Aim;
        using Aims = Perception::Aim::Type::Aims;
        using SectionAim = Perception::Aim::Type::SectionAim;

        using Log = Common::Log::System::Log;
        using VitalityStore = Blam::MapModel::Vitality::State::VitalityStore;
        using ObjectTableStore = Blam::Runtime::Object::State::ObjectTableStore;
        using BoneMatricesStore = Blam::Runtime::Object::State::BoneMatricesStore;
        using AimStore = Perception::Aim::State::AimStore;

    public:
        AimService(Log& log, VitalityStore& vitalityStore,
            ObjectTableStore& objectStore, BoneMatricesStore& boneMatricesStore,
            AimStore& aimStore) : m_Log(log),
            m_VitalityStore(vitalityStore), m_ObjectStore(objectStore),
            m_BoneMatricesStore(boneMatricesStore), m_AimStore(aimStore) {}
        ~AimService() = default;

        auto Update() -> void;

        auto Cleanup() -> void;

    private:
        Log& m_Log;
        VitalityStore& m_VitalityStore;
        ObjectTableStore& m_ObjectStore;
        BoneMatricesStore& m_BoneMatricesStore;
        AimStore& m_AimStore;

        static auto MakeAim(const ModelLinkAnchor& anchor, const AliveObject& object,
            const BoneMatrixTable* bones) -> SectionAim;

        static auto HasAnchors(const Vitality& layout) -> bool;
    };
}