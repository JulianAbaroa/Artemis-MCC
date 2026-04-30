#pragma once

#include <cstdint>
#include <vector>
#include <string>

// --- Spawn Points ---

struct ScnrSpawnPoint
{
	float Position[3];
	float FacingYaw;
	int16_t InsertionPointIndex;
};

// --- Trigger Volumes ---

enum class ScnrTriggerVolumeType : uint8_t
{
	BoundingBox = 0,
	Sector = 1,
};

struct ScnrTriggerVolume
{
	uint32_t NameId;
	float Position[3];
	float Extents[3];	// half-extents en X, Y, Z
	float Forward[3];
	float Up[3];
	ScnrTriggerVolumeType Type;
};

// --- Map Zones ---

struct ScnrMapZones
{ 
	std::vector<ScnrSpawnPoint> SpawnPoints;
	std::vector<ScnrTriggerVolume> KillZones;
	std::vector<ScnrTriggerVolume> SafeZones;
	std::vector<ScnrTriggerVolume> AllVolumes;
};