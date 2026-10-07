export module Resolved.Definitions.System:Mode;

import Common.Math.Type;
import Map.Reader.Type;
import Map.Tag.Type;
import Resolved.Definitions.Type;
import std;

export namespace Resolved::Definitions::System
{
    // Builds the render model definition.
    class ModeBuilder
    {
    private:
        using Vec3 = Common::Math::Type::Vec3;
        using Vec4 = Common::Math::Type::Vec4;
        using Node = Common::Math::Type::Node;
        using ModeObject = Map::Tag::Type::Mode::Object::ModeObject;
        using Mode = Resolved::Definitions::Type::Mode::Mode;
        using Marker = Resolved::Definitions::Type::Mode::Marker;
        using MarkerGroup = Resolved::Definitions::Type::Mode::MarkerGroup;
        using Bounds = Resolved::Definitions::Type::Mode::Bounds;

    public:
        ModeBuilder() = default;
        ~ModeBuilder() = default;

        // note: The model bounds come from the compressed position bounds of the compression info.
        auto Build(const ModeObject& mode) -> Mode;

    private:
        auto BuildMarkerGroups(const ModeObject& mode, Mode& out) -> void;
        auto BuildNodes(const ModeObject& mode, Mode& out) -> void;
        auto BuildBounds(const ModeObject& mode, Mode& out) -> void;

        auto MakeVec3(const Map::Reader::Type::Structure::Primitive::Vec3& v) -> Vec3;
        auto MakeVec4(const Map::Reader::Type::Structure::Primitive::Vec4& v) -> Vec4;
    };
}