export module Blam.MapModel.Definitions.System:Mode;

import Common.Math.Type;
import Blam.Map.Tag.Type;
import Blam.MapModel.Definitions.Type;
import std;

export namespace Blam::MapModel::Definitions::System
{
    // Builds the render model definition.
    class ModeBuilder
    {
    private:
        using Vec3 = Common::Math::Type::Vec3;
        using Node = Common::Math::Type::Node;
        using ModeObject = Blam::Map::Tag::Type::Mode::Object::ModeObject;
        using Mode = Blam::MapModel::Definitions::Type::Mode::Mode;
        using Marker = Blam::MapModel::Definitions::Type::Mode::Marker;
        using MarkerGroup = Blam::MapModel::Definitions::Type::Mode::MarkerGroup;
        using Region = Blam::MapModel::Definitions::Type::Mode::Region;
        using Bounds = Blam::MapModel::Definitions::Type::Mode::Bounds;

    public:
        ModeBuilder() = default;
        ~ModeBuilder() = default;

        // note: The model bounds come from the compressed position bounds of the compression info.
        auto Build(const ModeObject& mode) -> Mode;

    private:
        auto BuildMarkerGroups(const ModeObject& mode, Mode& out) -> void;
        auto BuildNodes(const ModeObject& mode, Mode& out) -> void;
        auto BuildRegions(const ModeObject& mode, Mode& out) -> void;
        auto BuildBounds(const ModeObject& mode, Mode& out) -> void;
    };
}