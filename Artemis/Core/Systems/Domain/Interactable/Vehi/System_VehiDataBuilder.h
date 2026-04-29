#pragma once

#include "Core/Types/Domain/Domains/Interactable/VehicleData.h"
#include "Generated/Vehi/VehiObject.h"

class System_VehiDataBuilder
{
public:
    VehicleData BuildData(const VehiObject& vehi);
};