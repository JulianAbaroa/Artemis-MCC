#pragma once

#include "Core/Types/Domain/Domains/TagProfile.h"
#include <cstdint>
#include <string>
#include <vector>

// Representation of an object inside the object Nodes graph.
struct ObjectNode
{
	uint32_t Handle;
	std::string TagName;
	std::string Class;

	float Position[3];
	float Forward[3];
	float Up[3];

	uint32_t ParentHandle = 0xFFFFFFFF;
	uint32_t ChildHandle = 0xFFFFFFFF;
	uint32_t NextSiblingHandle = 0xFFFFFFFF;

	// Basis for the classification and derivation
	// of behaviors in InteractableSystem.
	TagProfile Profile;
};

// Representation of a player inside the player object Tree.
struct PlayerObjectTree
{
	uint32_t PlayerHandle;
	std::string Gamertag;

	uint32_t BipedHandle = 0xFFFFFFFF;
	uint32_t PrimaryWeaponHandle = 0xFFFFFFFF;
	uint32_t SecondaryWeaponHandle = 0xFFFFFFFF;
	uint32_t VehicleHandle = 0xFFFFFFFF;
	uint32_t ObjectiveHandle = 0xFFFFFFFF;

	std::vector<uint32_t> VehicleSiblingHandles;
};