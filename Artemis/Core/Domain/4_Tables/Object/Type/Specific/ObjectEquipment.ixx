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
		// Energy left of an armor ability, from 0 (depleted) to 1 (full).
		float Energy{};

		// Whether a sustained armor ability is running (camo, armor lock, jetpack, sprint).
		// note: Drop shield, hologram and evade are instant and never read active.
		bool IsActive{};
		CrateKind Kind{ CrateKind::Unknown };
		std::optional<CrateShield> Shield{};
	};
}