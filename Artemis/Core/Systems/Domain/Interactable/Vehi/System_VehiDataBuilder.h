#pragma once

struct VehicleData;
struct VehiObject;

class System_VehiDataBuilder
{
public:
    VehicleData BuildData(const VehiObject& vehi);
};