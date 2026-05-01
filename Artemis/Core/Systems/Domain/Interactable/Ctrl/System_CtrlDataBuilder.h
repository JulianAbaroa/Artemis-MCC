#pragma once

struct ControlDeviceData;
struct CtrlObject;

class System_CtrlDataBuilder
{
public:
    ControlDeviceData BuildData(const CtrlObject& ctrl);
};