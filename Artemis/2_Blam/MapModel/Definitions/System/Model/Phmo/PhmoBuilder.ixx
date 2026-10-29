export module Blam.MapModel.Definitions.System:Phmo;

import Blam.Map.Tag.Type;
import Blam.MapModel.Definitions.Type;
import std;

export namespace Blam::MapModel::Definitions::System
{
    // Builds the physics model definition.
    class PhmoBuilder
    {
    private:
        using PhmoObject = Blam::Map::Tag::Type::Phmo::Object::PhmoObject;
        using Phmo = Blam::MapModel::Definitions::Type::Phmo::Phmo;
        using RigidBody = Blam::MapModel::Definitions::Type::Phmo::RigidBody;
        using ShapeType = Blam::MapModel::Definitions::Type::Phmo::ShapeType;

    public:
        PhmoBuilder() = default;
        ~PhmoBuilder() = default;

        // note: Expands the root shape of each rigid body down to its primitive shapes.
        auto Build(const PhmoObject& phmo) -> Phmo;

    private:
        // Appends the primitive shapes under one shape, following lists, mopps and phantoms.
        // param listStarts: First ListShapes entry of each list. The lists take their children in order from that block.
        // param isPhantom: True below a phantom shape.
        // param depth: Nesting level, to stop on a malformed tag.
        auto AppendShapes(const PhmoObject& phmo, const std::vector<std::size_t>& listStarts, ShapeType type,
            std::int16_t index, bool isPhantom, int depth, RigidBody& out) -> void;
    };
}