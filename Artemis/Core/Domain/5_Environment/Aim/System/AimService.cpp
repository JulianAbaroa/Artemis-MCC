module Environment.Aim.System;

namespace
{
    using Vec3 = Common::Math::Type::Vec3;
    using AnchorSource = Resolved::World::Type::ModelLink::AnchorSource;
}

namespace Environment::Aim::System
{
    auto AimService::Update() -> void
    {
        auto objectTablePtr = m_ObjectStore.Acquire();
        if (!objectTablePtr) return;

        const ObjectTable& objectTable = *objectTablePtr;

        Aims result;

        for (const auto& [handle, object] : objectTable)
        {
            if (object.Address == 0) continue;

            const ResolvedVitality* layout = m_VitalityStore.Get(object.TagName);
            if (!layout || !this->HasAnchors(*layout)) continue;

            const BoneMatrixTable* bones = m_BoneMatricesStore.Get(object.Handle);

            Aim aim;
            aim.Handle = object.Handle;
            aim.Sections.reserve(layout->Sections.size());

            for (const auto& section : layout->Sections)
            {
                aim.Sections.push_back(this->MakeAim(section.Aim, object, bones));
            }

            result.emplace(object.Handle, std::move(aim));
        }

        m_AimStore.Publish(std::move(result));
    }

    auto AimService::MakeAim(const AimAnchor& anchor, const AliveObject& object,
        const BoneMatrixTable* bones) -> SectionAim
    {
        SectionAim aim;
        aim.Radius = anchor.Radius;
        aim.AimSource = anchor.Source;

        if (anchor.Source == AnchorSource::None) return aim;

        const Vec3& offset = anchor.LocalOffset;

        if (anchor.ModelNodeIndex >= 0)
        {
            if (!bones ||
                static_cast<std::size_t>(anchor.ModelNodeIndex) >= bones->Matrices.size())
            {
                return aim;
            }

            const auto& bone = bones->Matrices[anchor.ModelNodeIndex];
            if (!Tables::Object::Type::BoneMatrix::IsBoneMatrixValid(bone)) return aim;

            aim.Position = Tables::Object::Type::BoneMatrix::Transform(bone, offset.X, offset.Y, offset.Z);
            aim.Valid = true;
            return aim;
        }

        // Object space (center of the model): origin = Position, axes = forward, left, up
        // Note: rigid objects (no bones) place their collision centered on its bounds,
        // so this may not match them. Not handled yet.
        const Vec3& f = object.Forward;
        const Vec3& u = object.Up;

        const float lx = u.Y * f.Z - u.Z * f.Y;
        const float ly = u.Z * f.X - u.X * f.Z;
        const float lz = u.X * f.Y - u.Y * f.X;

        aim.Position = Vec3{
            object.Position.X + offset.X * f.X + offset.Y * lx + offset.Z * u.X,
            object.Position.Y + offset.X * f.Y + offset.Y * ly + offset.Z * u.Y,
            object.Position.Z + offset.X * f.Z + offset.Y * lz + offset.Z * u.Z
        };
        aim.Valid = true;
        return aim;
    }

    auto AimService::HasAnchors(const ResolvedVitality& layout) -> bool
    {
        for (const auto& section : layout.Sections)
        {
            if (section.Aim.Source != AnchorSource::None) return true;
        }
        return false;
    }

    auto AimService::Cleanup() -> void
    {
        m_AimStore.Cleanup();

        m_LogsService.Message("[AimService] INFO: Cleanup completed.");
    }
}