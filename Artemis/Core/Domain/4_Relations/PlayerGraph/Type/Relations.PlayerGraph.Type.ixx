export module Relations.PlayerGraph.Type;

import std;

export namespace Relations::PlayerGraph::Type
{
	struct PlayerTree
	{
		std::uint32_t Handle;

		std::uint32_t BipedHandle = 0xFFFFFFFF;
		std::uint32_t PrimaryWeaponHandle = 0xFFFFFFFF;
		std::uint32_t SecondaryWeaponHandle = 0xFFFFFFFF;
		std::uint32_t AbilityHandle = 0xFFFFFFFF;
		std::uint32_t ObjectiveHandle = 0xFFFFFFFF;
		std::uint32_t ParentHandle = 0xFFFFFFFF;

		std::vector<std::uint32_t> VehiclePartHandles;

		bool IsAlive() const
		{
			return BipedHandle != 0xFFFFFFFF;
		}

		bool IsInVehicle() const
		{
			return ParentHandle != 0xFFFFFFFF;
		}
	};
}