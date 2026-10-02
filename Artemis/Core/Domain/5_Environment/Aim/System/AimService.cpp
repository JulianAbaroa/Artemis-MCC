module Environment.Aim.System;

namespace
{
    using Vec3 = Common::Math::Type::Vec3;
    using AimSource = Resolved::World::Type::ModelLink::AnchorSource;
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

            // Most objects have no damage sections / vitality layout.
            const ResolvedVitality* layout = m_VitalityStore.GetResolvedVitality(object.TagName);
            if (!layout || !this->HasAnchors(*layout)) continue;

            // May be null (no bones): node anchors are then marked invalid.
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
        aim.Source = anchor.Source;

        if (anchor.Source == AimSource::None) return aim;

        const Vec3& offset = anchor.LocalOffset;

        if (anchor.ModelNodeIndex >= 0)
        {
            // Node space: bone matrix takes node-local points to world
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

        // left = up x forward
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
            if (section.Aim.Source != AimSource::None) return true;
        }
        return false;
    }

    auto AimService::Cleanup() -> void
    {
        m_AimStore.Clear();

        m_LogsService.Message("[AimService] INFO: Cleanup completed.");
    }
}