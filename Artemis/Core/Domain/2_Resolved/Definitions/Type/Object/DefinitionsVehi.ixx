export module Resolved.Definitions.Type:Vehi;

import :Object;
import Common.Math.Type;
import std;

namespace
{
    using Vec3 = Common::Math::Type::Vec3;
    using Vec4 = Common::Math::Type::Vec4;
    using ResolvedObject = Resolved::Definitions::Type::Object::Object;
}

export namespace Resolved::Definitions::Type::Vehi
{
    enum class SeatKind : std::uint8_t
    {
        None = 0x00,
        Passenger = 0x01,
        Gunner = 0x02,
        SmallCargo = 0x03,
        LargeCargo = 0x04,
        Driver = 0x05,

        Invalid = 0xFF,
    };

    struct Seat
    {
        std::uint32_t SeatMarkerNameId{ 0 };
        std::int8_t NodeIndex{};
        SeatKind Kind{ SeatKind::Invalid };
        Vec3 LocalTranslation{};
        Vec4 LocalRotation{};
        float EntryRadius{ 0.0f };
    };

    struct Vehi
    {
        ResolvedObject Base{};

        std::string TagName{};
        std::vector<Seat> Seats{};
    };
}