#include "pch.h"
#include "Core/Common/MapMagics.h"
#include "Core/States/CoreState.h"
#include "Core/States/Domain/CoreDomainState.h"
#include "Core/States/Domain/Tables/ObjectTableState.h"
#include "Core/States/Domain/Map/MapState.h"
#include "Core/States/Domain/Map/Sbsp/MapSbspState.h"
#include "Core/States/Domain/Navigation/NavigationState.h"
#include "Core/Systems/CoreSystem.h"
#include "Core/Systems/Domain/CoreDomainSystem.h"
#include "Core/Systems/Domain/Map/Sbsp/SbspGeometryBuilder.h"
#include "Core/Systems/Domain/Map/Sbsp/SbspSeamLinker.h"
#include "Core/Systems/Domain/Navigation/NavigationSystem.h"
#include "Core/Systems/Interface/DebugSystem.h"

void NavigationSystem::BuildForMap()
{
	const int32_t tagCount =
		static_cast<int32_t>(g_pState->Domain->Map->GetTagsSize());

	std::vector<SbspGeometry> geometries;
	std::vector<const SbspObject*> sbspObjects;

	int32_t builtCount = 0;

	for (int32_t i = 0; i < tagCount; ++i)
	{
		const Map_TagTableEntry& entry = g_pState->Domain->Map->GetTag(i);
		if (entry.TagGroupIndex < 0) continue;

		const uint32_t magic =
			g_pState->Domain->Map->GetGroupMagic(entry.TagGroupIndex);
		if (magic != MapMagics::k_SbspMagic) continue;

		const std::string tagName = g_pState->Domain->Map->GetTagName(i);
		if (tagName.empty()) continue;

		if (tagName.find("shared") != std::string::npos) continue;
		if (tagName.find("hidden") != std::string::npos) continue;

		const SbspObject* sbsp = g_pState->Domain->MapSbsp->GetSbsp(tagName);
		if (!sbsp)
		{
			g_pSystem->Debug->Log("[NavigationSystem] WARNING:"
				" SBSP tag found in table but not loaded: ", tagName);
			continue;
		}

		SbspGeometry geometry = 
			g_pSystem->Domain->GeometryBuilder->BuildGeometry(*sbsp);

		g_pSystem->Debug->Log("[NavigationSystem] INFO:"
			" Built %d clusters, %d portals, %d markers, %d seams.",
			geometry.Clusters.size(), geometry.Portals.size(), 
			geometry.Markers.size(), sbsp->StructureSeams.size());

		geometries.push_back(std::move(geometry));
		sbspObjects.push_back(sbsp);
		++builtCount;
	}

	// Link clusters across BSP boundaries via Structure Seam centroid matching.
	// This mutates CrossLinks inside each geometry's clusters.
	if (builtCount > 1)
	{
		g_pSystem->Domain->SeamLinker->LinkSeams(geometries, sbspObjects);

		// Log how many cross-links were produced total.
		int32_t totalCrossLinks = 0;
		for (const auto& geometry : geometries)
		{
			for (const auto& cluster : geometry.Clusters)
			{
				totalCrossLinks += static_cast<int32_t>(cluster.CrossLinks.size());
			}
		}

		g_pSystem->Debug->Log("[NavigationSystem] INFO:"
			" Seam linking complete. Total cross-links: %d", totalCrossLinks);
	}

	// Push all geometries into state.
	for (auto& geometry : geometries)
	{
		g_pState->Domain->Navigation->AddGeometry(geometry);
	}

	g_pSystem->Debug->Log("[NavigationSystem] INFO:"
		" Navigation built. Total SBSPs: %d", builtCount);
}

void NavigationSystem::Cleanup()
{
	g_pState->Domain->Navigation->Cleanup();
	g_pSystem->Debug->Log("[NavigationSystem] INFO: Cleanup completed.");
}