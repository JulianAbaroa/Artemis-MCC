export module Map.Reader.Type;

export import :Structure;

import Common.Math.System;
import Common.Math.Type;
import std;

export namespace Map::Reader::Type
{
    // Returns the length of a map vector.
    inline auto LengthOf(const Structure::Primitive::Vec3& v) -> float
    {
        return Common::Math::System::Length(Common::Math::Type::Vec3{ v.X, v.Y, v.Z });
    }
}