export module DevTools.Gui.Format.System:Fixtures;

import Common.ZoneShape.Type;

export namespace DevTools::Gui::Format::System
{
    // Converts the fixture enums to display text. Unlisted values give "Unknown".
    class FixturesFormater
    {
    private:
        using ZoneShapeKind = Common::ZoneShape::Type::ZoneShapeKind;

    public:
        FixturesFormater() = default;
        ~FixturesFormater() = default;

        static auto ZoneKindToString(ZoneShapeKind kind) -> const char*;
    };
}