#include "pch.h"
#include "Core/Systems/Domain/Navigation/Bloc/System_BlocObstacleBuilder.h"
#include "Core/Types/Domain/Domains/Navigation/BlocObstacleData.h"
#include "Generated/Bloc/BlocObject.h"

CrateObstacleData System_BlocObstacleBuilder::BuildData(const BlocObject& bloc) const
{
    CrateObstacleData out{};
    const auto& d = bloc.Data;

    out.TagName = bloc.TagName;
    out.BoundingRadius = d.BoundingRadius;
    out.NotAPathfindingObstacle = (d.Flags & (1u << 3)) != 0;

    out.HasAiProperties = !bloc.AiProperties.empty();
    if (out.HasAiProperties)
    {
        const auto& ai = bloc.AiProperties[0];
        out.IsDestroyableCover = (ai.Flags & (1u << 0)) != 0;
        out.IsDynamicCover = (ai.Flags & (1u << 2)) != 0;
        out.IsDynamicCoverFromCentre = (ai.Flags & (1u << 4)) != 0;
        out.HasCornerMarkers = (ai.Flags & (1u << 5)) != 0;
        out.AiSize = static_cast<BlocAiSize>(ai.AiSize);
    }

    return out;
}

bool System_BlocObstacleBuilder::IsObstacleRelevant(const BlocObject& bloc) const
{
    // Ignore if explicitly not a pathfinding obstacle and has no cover data
    const bool notPathfinding = (bloc.Data.Flags & (1u << 3)) != 0;
    const bool hasCover = !bloc.AiProperties.empty();
    return !notPathfinding || hasCover;
}