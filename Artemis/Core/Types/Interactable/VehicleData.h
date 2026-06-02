#pragma once

#include <cstdint>
#include <string>
#include <vector>

enum class VehicleAISeatType : uint8_t
{
    None = 0,
    Passenger = 1,
    Gunner = 2,
    SmallCargo = 3,
    LargeCargo = 4,
    Driver = 5,
};

struct VehicleSeat
{
    uint32_t SeatMarkerNameId = 0;
    uint32_t EntryMarkerNameId = 0;

    VehicleAISeatType AISeatType = VehicleAISeatType::None;

    float EntryRadius = 0.0f;

    bool IsDriver = false;
    bool IsGunner = false;
    bool HasMarker = false;
    bool InvalidForPlayer = false;
    bool InvalidForNonPlayer = false;
};

struct VehicleData
{
    std::string TagName;
    std::vector<VehicleSeat> Seats;
};