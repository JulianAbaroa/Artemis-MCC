export module Gui.Format.System:Fixtures;

import Common.ZoneShape.Type;

export namespace Gui::Format::System
{
    // Converts the fixture enums to display text. Unlisted values give "Unknown".
    class FixturesFormater
    {
    private:
        using ZoneKind = Common::ZoneShape::Type::Kind;

    public:
        FixturesFormater() = default;
        ~FixturesFormater() = default;

        static auto ZoneKindToString(ZoneKind kind) -> const char*;
    };
}