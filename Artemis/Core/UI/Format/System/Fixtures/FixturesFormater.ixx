export module UI.Format.System:Fixtures;

import Common.ZoneShape.Type;
import Tables.Object.Type;
import Environment.Fixtures.Type;

export namespace UI::Format::System
{
	class FixturesFormater
	{
	private:
		using ZoneKind = Common::ZoneShape::Type::Kind;
		using AngleKind = Tables::Object::Type::Crate::Lift::Angle;
		using ForceKind = Tables::Object::Type::Crate::Lift::Force;
		using DestructibleKind = Environment::Fixtures::Type::Destructible::Kind;
		using TeleporterKind = Environment::Fixtures::Type::Teleport::Kind;

	public:
		FixturesFormater() = default;
		~FixturesFormater() = default;

		static auto ZoneKindToString(ZoneKind kind) -> const char*;
		static auto AngleKindToString(AngleKind kind) -> const char*;
		static auto ForceKindToString(ForceKind kind) -> const char*;
		static auto DestructibleKindToString(DestructibleKind kind) -> const char*;
		static auto TeleporterKindToString(TeleporterKind kind) -> const char*;
	};
}