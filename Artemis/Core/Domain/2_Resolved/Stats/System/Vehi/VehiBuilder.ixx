export module Resolved.Stats.System:Vehi;

import Common.Math.Type;
import Map.Tag.Type;
import Resolved.Stats.Type;

export namespace Resolved::Stats::System
{
    class VehiBuilder
    {
    private:
        using VehiObject = Map::Tag::Type::Vehi::Object::VehiObject;
        using ResolvedVehi = Resolved::Stats::Type::Vehi::Vehi;
        
    public:
        VehiBuilder() = default;
        ~VehiBuilder() = default;

        auto Build(const VehiObject& vehi) -> ResolvedVehi;
    };
}