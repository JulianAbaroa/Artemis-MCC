module Views.Raycast.System;

import Common.Math.Type;
import Blam.MapModel.Definitions.Type;
import Blam.Runtime.Object.Type;

namespace
{
    using Common::Math::Type::Vec3;
    using Blam::MapModel::Definitions::Type::Bipd::Bipd;
    using Blam::Runtime::Object::Type::Alive::AliveObject;
    using Blam::Runtime::Object::Type::BoneMatrix::BoneMatrix;
    using Blam::Runtime::Object::Type::BoneMatrix::BoneMatrixTable;
    using Blam::Runtime::Object::Type::BoneMatrix::FindValidBone;
    using Views::Raycast::Type::EyePoint;
}

namespace Views::Raycast::System
{
    auto DerivedEye(const AliveObject& biped, const Bipd& definition, float crouchFraction) -> Vec3
    {
        const float height{ definition.StandingCameraHeight + (definition.CrouchingCameraHeight - definition.StandingCameraHeight) * crouchFraction };
        return Vec3{
            biped.WorldOrigin.X + biped.Up.X * height,
            biped.WorldOrigin.Y + biped.Up.Y * height,
            biped.WorldOrigin.Z + biped.Up.Z * height
        };
    }

    auto HeadBoneEye(const BoneMatrixTable& bones, std::int16_t headNode) -> EyePoint
    {
        const BoneMatrix* bone{ FindValidBone(bones.Matrices, headNode) };
        if (!bone) return {};

        return EyePoint{ Vec3{ bone->Translation[0], bone->Translation[1], bone->Translation[2] }, true };
    }
}