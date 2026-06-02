#pragma once

struct VehicleData;
struct VehiObject;

class System_VehiBuilder
{
public:
    VehicleData Build(const VehiObject& vehi);
};