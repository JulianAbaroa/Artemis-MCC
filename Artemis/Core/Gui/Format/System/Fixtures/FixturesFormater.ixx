export module Gui.Format.System:Fixtures;

import Common.ZoneShape.Type;
import Environment.Fixtures.Type;

export namespace Gui::Format::System
{
    // Converts the fixture enums to display text. Unlisted values give "Unknown".
    class FixturesFormater
    {
    private:
        using ZoneKind = Common::ZoneShape::Type::Kind;
        using DestructibleKind = Environment::Fixtures::Type::Destructible::Kind;

    public:
        FixturesFormater() = default;
        ~FixturesFormater() = default;

        static auto ZoneKindToString(ZoneKind kind) -> const char*;
        static auto DestructibleKindToString(DestructibleKind kind) -> const char*;
    };
}