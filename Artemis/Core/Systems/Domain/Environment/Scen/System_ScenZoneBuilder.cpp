#include "pch.h"
#include "Core/Systems/Domain/Environment/Scen/System_ScenZoneBuilder.h"
#include "Core/Types/Domain/Domains/Environment/SceneryZoneData.h"
#include "Generated/Scen/ScenObject.h"

SceneryZoneData System_ScenZoneBuilder::BuildData(const ScenObject& scen) const
{
    SceneryZoneData out{};
    const auto& mp = scen.MultiplayerObject[0];

    out.TagName = scen.TagName;
    out.MpZoneTypeRaw = mp.Type;
    out.BoundaryShape = static_cast<ScenBoundaryShape>(mp.BoundaryShape);
    out.BoundaryWidthRadius = mp.BoundaryWidthRadius;
    out.BoundaryBoxLength = mp.BoundaryBoxLength;
    out.BoundaryHeight = mp.BoundaryHeight;
    out.IsValidInitialSpawn = (mp.Flags & (1u << 2)) != 0;

    return out;
}

bool System_ScenZoneBuilder::IsMpZone(const ScenObject& scen) const
{
    return !scen.MultiplayerObject.empty();
}