export module UI.Format.System:Object;

import Tables.Object.Type;

export namespace UI::Format::System
{
	class ObjectFormater
	{
	private:
		using ActionState = Tables::Object::Type::Weapon::ActionState;
		using VehicleKind = Tables::Object::Type::Vehicle::Kind;
		using ZoomLevel = Tables::Object::Type::Biped::ZoomLevel;
		using CrateKind = Tables::Object::Type::Crate::Kind;
		using SceneryKind = Tables::Object::Type::Scenery::Kind;

	public:
		ObjectFormater() = default;
		~ObjectFormater() = default;

		static auto ActionStateToString(ActionState state) -> const char*;
		static auto VehicleKindToString(VehicleKind kind) -> const char*;
		static auto ZoomLevelToString(ZoomLevel zoom) -> const char*;
		static auto CrateKindToString(CrateKind kind) -> const char*;
		static auto SceneryKindToString(SceneryKind kind) -> const char*;
	};
}