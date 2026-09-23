export module Resolved.World.System:SbspSeamLinker;

import Common.Math.Type;
import Map.Tag.Type;
import Resolved.World.Type;
import std;

export namespace Resolved::World::System
{
    class SbspSeamLinker
    {
    private:
        using Vec3 = Common::Math::Type::Vec3;
        using SbspObject = Map::Tag::Type::Sbsp::Object::SbspObject;
        using ResolvedSbsp = Resolved::World::Type::Sbsp::Sbsp;

    public:
        SbspSeamLinker() = default;
        ~SbspSeamLinker() = default;

        auto LinkSeams(std::vector<ResolvedSbsp>& geometries,
            const std::vector<const SbspObject*>& sbspObjects,
            float threshold = 0.05f) -> void;

    private:
        // --- Helpers ---
        auto DistanceSquared(const Vec3& a, const Vec3& b) -> float;
    };
}