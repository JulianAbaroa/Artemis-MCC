#pragma once

#include <cstdint>

// Representation of 'TagGroups', game object's classes.
enum class ObjectClass : uint8_t
{
	Invalid = 0xFF,

	Biped = 0x00,				// "bipd"
	Vehicle = 0x01,				// "vehi"
	Weapon = 0x02,				// "weap"
	Equipment = 0x03,			// "eqip"
	Projectile = 0x05,			// "proj"
	Scenery = 0x06,				// "scen"
	DeviceMachine = 0x07,		// "mach"
	DeviceControl = 0x08,		// "ctrl"
	SoundScenery = 0x09,		// "ssce"
	Crate = 0x0A,				// "bloc"

	// Unknown.
	ScenarioStructureBsp,		// "sbsp"
	CollisionModel,				// "coll"
	RenderModel,				// "mode"
	PhysicsModel,				// "phmo"
	Scenario,					// "scnr"
};