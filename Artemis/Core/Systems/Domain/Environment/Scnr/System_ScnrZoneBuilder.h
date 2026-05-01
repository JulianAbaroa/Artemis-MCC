#pragma once

#include <cstdint>

struct Scnr_TriggerVolumesObject;
struct ScnrTriggerVolume;
struct ScnrMapZones;
struct ScnrObject;

class System_ScnrZoneBuilder
{
public:
    ScnrMapZones BuildZone(const ScnrObject& scnr);

private:
    void BuildSpawnPoints(const ScnrObject& scnr, ScnrMapZones& out);
    void BuildTriggerVolumes(const ScnrObject& scnr, ScnrMapZones& out);
    void BuildNamedLocationVolumes(const ScnrObject& scnr, ScnrMapZones& out);

    // --- Helpers ---

    float ExtractYawFromDegree2(uint32_t degree2);
    ScnrTriggerVolume BuildTriggerVolume(const Scnr_TriggerVolumesObject& tv, bool onlyKillPlayers);
};