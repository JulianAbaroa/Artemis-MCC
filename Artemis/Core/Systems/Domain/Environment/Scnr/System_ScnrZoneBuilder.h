#pragma once

#include "Core/Types/Domain/Domains/Environment/ScnrZones.h"
#include "Generated/Scnr/ScnrObject.h"

struct ScnrObject;
struct ScnrMapZones;

class System_ScnrZoneBuilder
{
public:
	ScnrMapZones BuildZone(const ScnrObject& scnr);

private:
	void BuildSpawnPoints(const ScnrObject& scnr, ScnrMapZones& out);
	void BuildTriggerVolumes(const ScnrObject& scnr, ScnrMapZones& out);

	// --- Helpers ---

	float ExtractYawFromDegree2(uint32_t degree2);
	ScnrTriggerVolume BuildTriggerVolume(const Scnr_TriggerVolumesObject& tv);
};