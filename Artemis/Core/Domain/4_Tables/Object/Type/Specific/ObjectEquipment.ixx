export module Tables.Object.Type:Equipment;

import :Crate;
import std;

namespace
{
	using CrateKind = Tables::Object::Type::Crate::Kind;
	using CrateShield = Tables::Object::Type::Crate::Shield::Shield;
}

export namespace Tables::Object::Type::Equipment
{
	struct Equipment
	{
		float Energy{};
		CrateKind Kind{ CrateKind::Unknown };
		std::optional<CrateShield> Shield{};
	};
}