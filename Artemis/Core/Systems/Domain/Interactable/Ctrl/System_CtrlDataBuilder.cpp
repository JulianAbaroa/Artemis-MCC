#include "pch.h"
#include "Core/Systems/Domain/Interactable/Ctrl/System_CtrlDataBuilder.h"
#include "Core/Types/Domain/Domains/Interactable/ControlDeviceData.h"
#include "Generated/Ctrl/CtrlObject.h"

ControlDeviceData System_CtrlDataBuilder::BuildData(const CtrlObject& ctrl)
{
    ControlDeviceData out{};
    const auto& d = ctrl.Data;

    out.TagName = ctrl.TagName;

    out.DeviceType = static_cast<CtrlDeviceType>(d.Type);
    out.AutomaticActivationRadius = d.AutomaticActivationRadius;
    out.BoundingRadius = d.BoundingRadius;

    out.PowerTransitionTime = d.PowerTransitionTime;
    out.PowerAccelerationTime = d.PowerAccelerationTime;
    out.PositionTransitionTime = d.PositionTransitionTime;
    out.PositionAccelerationTime = d.PositionAccelerationTime;

    if (!ctrl.MultiplayerObject.empty())
    {
        const auto& mp = ctrl.MultiplayerObject[0];
        out.DefaultSpawnTime = mp.DefaultSpawnTime;
        out.DefaultAbandonmentTime = mp.DefaultAbandonmentTime;
    }

    return out;
}