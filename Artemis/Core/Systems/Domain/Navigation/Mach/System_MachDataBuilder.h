#pragma once

struct MachineData;
struct MachObject;

class System_MachDataBuilder
{
public:
    MachineData BuildData(const MachObject& mach) const;
};