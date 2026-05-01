#include "pch.h"
#include "Core/Systems/Domain/Navigation/Bloc/System_BlocTeleporterBuilder.h"
#include "Core/Types/Domain/Domains/Navigation/BlocTeleporterData.h"
#include "Generated/Bloc/BlocObject.h"

BlocTeleporterData System_BlocTeleporterBuilder::BuildData(const BlocObject& bloc) const
{
    BlocTeleporterData out{};
    const auto& mp = bloc.MultiplayerObject[0];

    out.TagName = bloc.TagName;
    out.Role = static_cast<BlocTeleporterRole>(mp.Type);
    out.BoundaryShape = static_cast<BlocBoundaryShape>(mp.BoundaryShape);
    out.BoundaryWidthRadius = mp.BoundaryWidthRadius;
    out.BoundaryBoxLength = mp.BoundaryBoxLength;
    out.BoundaryHeight = mp.BoundaryHeight;
    out.DefaultSpawnTime = mp.DefaultSpawnTime;
    out.DefaultAbandonmentTime = mp.DefaultAbandonmentTime;

    return out;
}

bool System_BlocTeleporterBuilder::IsTeleporter(const BlocObject& bloc) const
{
    if (bloc.MultiplayerObject.empty())
        return false;

    const uint8_t type = bloc.MultiplayerObject[0].Type;
    return type == 12 || type == 13 || type == 14;
}