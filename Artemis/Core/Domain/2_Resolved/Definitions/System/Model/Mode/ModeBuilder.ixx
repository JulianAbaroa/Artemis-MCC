export module Resolved.Definitions.System:Mode;

import Common.Math.Type;
import Map.Reader.Type;
import Map.Tag.Type;
import Resolved.Definitions.Type;
import std;

export namespace Resolved::Definitions::System
{
    class ModeBuilder
    {
    private:
        using Vec3 = Common::Math::Type::Vec3;
        using Vec4 = Common::Math::Type::Vec4;
        using Node = Common::Math::Type::Node;
        using ModeObject = Map::Tag::Type::Mode::Object::ModeObject;
        using ResolvedMode = Resolved::Definitions::Type::Mode::Mode;
        using Marker = Resolved::Definitions::Type::Mode::Marker;
        using MarkerGroup = Resolved::Definitions::Type::Mode::MarkerGroup;
        using Bounds = Resolved::Definitions::Type::Mode::Bounds;

    public:
        ModeBuilder() = default;
        ~ModeBuilder() = default;

        auto Build(const ModeObject& mode) -> ResolvedMode;

    private:
        auto BuildMarkerGroups(const ModeObject& mode, ResolvedMode& out) -> void;
        auto BuildNodes(const ModeObject& mode, ResolvedMode& out) -> void;
        auto BuildBounds(const ModeObject& mode, ResolvedMode& out) -> void;

        auto MakeVec3(const Map::Reader::Type::Structure::Primitive::Vec3& v) -> Vec3;
        auto MakeVec4(const Map::Reader::Type::Structure::Primitive::Vec4& v) -> Vec4;
    };
}