#pragma once

#include <string>
#include <cstdint>

enum class CtrlDeviceType : uint16_t
{
    Toggle = 0,
    On = 1,
    Off = 2,
    Call = 3,
    Health = 4,
};

struct ControlDeviceData
{
    std::string TagName;

    // What kind of device this is
    CtrlDeviceType  DeviceType;

    // Proximity: radius at which the device auto-activates (0 = manual only)
    float AutomaticActivationRadius;
    float BoundingRadius;

    // Transition timings — how long the device takes to move/power
    // Useful for AI to know if the station is mid-transition (unavailable)
    float PowerTransitionTime;
    float PowerAccelerationTime;
    float PositionTransitionTime;
    float PositionAccelerationTime;

    // Multiplayer spawn data (0 if not applicable)
    int16_t DefaultSpawnTime;
    int16_t DefaultAbandonmentTime;
};