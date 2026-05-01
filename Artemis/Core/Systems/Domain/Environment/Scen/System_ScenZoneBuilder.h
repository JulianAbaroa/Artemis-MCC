#pragma once

struct SceneryZoneData;
struct ScenObject;

class System_ScenZoneBuilder
{
public:
    SceneryZoneData BuildData(const ScenObject& scen) const;
    // Returns false if this scen has no MP zone block
    bool IsMpZone(const ScenObject& scen) const;
};