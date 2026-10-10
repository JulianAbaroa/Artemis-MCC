export module Environment.Aim.System;

import Service.Logs.System;
import Common.Math.Type;
import Resolved.World.Type;
import Resolved.Vitality.Type;
import Resolved.Vitality.State;
import Tables.Object.Type;
import Tables.Object.State;
import Environment.Aim.Type;
import Environment.Aim.State;
import std;

export namespace Environment::Aim::System
{
    class AimService
    {
    private:
        using Vec3 = Common::Math::Type::Vec3;
        using AliveObject = Tables::Object::Type::Alive::Object;
        using ObjectTable = std::unordered_map<std::uint32_t, AliveObject>;
        using BoneMatrixTable = Tables::Object::Type::BoneMatrix::BoneMatrixTable;
        using ResolvedVitality = Resolved::Vitality::Type::Vitality::Vitality;
        using AimAnchor = Resolved::World::Type::ModelLink::Anchor;
        using Aim = Environment::Aim::Type::Aim;
        using Aims = std::unordered_map<std::uint32_t, Environment::Aim::Type::Aim>;
        using SectionAim = Environment::Aim::Type::SectionAim;

        using LogsService = Service::Logs::System::LogsService;
        using VitalityStore = Resolved::Vitality::State::VitalityStore;
        using ObjectTableStore = Tables::Object::State::ObjectTableStore;
        using BoneMatricesStore = Tables::Object::State::BoneMatricesStore;
        using AimStore = Environment::Aim::State::AimStore;

    public:
        AimService(LogsService& logsService, VitalityStore& vitalityStore,
            ObjectTableStore& objectStore, BoneMatricesStore& boneMatricesStore,
            AimStore& aimStore) : m_LogsService(logsService),
            m_VitalityStore(vitalityStore), m_ObjectStore(objectStore),
            m_BoneMatricesStore(boneMatricesStore), m_AimStore(aimStore) {}
        ~AimService() = default;

        auto Update() -> void;

        auto Cleanup() -> void;

    private:
        LogsService& m_LogsService;
        VitalityStore& m_VitalityStore;
        ObjectTableStore& m_ObjectStore;
        BoneMatricesStore& m_BoneMatricesStore;
        AimStore& m_AimStore;

        static auto MakeAim(const AimAnchor& anchor, const AliveObject& object,
            const BoneMatrixTable* bones) -> SectionAim;

        static auto HasAnchors(const ResolvedVitality& layout) -> bool;
    };
}