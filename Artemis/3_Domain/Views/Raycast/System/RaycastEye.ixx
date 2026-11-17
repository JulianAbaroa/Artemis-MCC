export module Views.Raycast.System:Eye;

import Common.Math.Type;
import Blam.MapModel.Definitions.Type;
import Blam.Runtime.Object.Type;
import Views.Raycast.Type;
import std;

// Where an agent looks from, valid for players with and without a camera (F4.B, D309 proposal).

namespace
{
    using Common::Math::Type::Vec3;
    using Blam::MapModel::Definitions::Type::Bipd::Bipd;
    using Blam::Runtime::Object::Type::Alive::AliveObject;
    using Blam::Runtime::Object::Type::BoneMatrix::BoneMatrixTable;
    using Views::Raycast::Type::EyePoint;
}

export namespace Views::Raycast::System
{
    // return: The eye of a biped from its tag (A): the origin of the biped plus its up vector times the camera height, which goes
    // from the standing height of the tag to the crouching one with crouchFraction (0 to 1).
    auto DerivedEye(const AliveObject& biped, const Bipd& definition, float crouchFraction) -> Vec3;

    // return: The eye of a biped at the node of its head (B), or an invalid one when the node is out of range or its matrix is not valid.
    auto HeadBoneEye(const BoneMatrixTable& bones, std::int16_t headNode) -> EyePoint;
}