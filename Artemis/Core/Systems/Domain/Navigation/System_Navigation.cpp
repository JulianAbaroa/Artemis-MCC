#include "pch.h"
#include "Core/Common/Map/MapMagics.h"
#include "Core/States/Core_State.h"
#include "Core/States/Domain/Core_State_Domain.h"
#include "Core/States/Domain/Tables/State_ObjectTable.h"
#include "Core/States/Domain/Map/State_Map.h"
#include "Core/States/Domain/Map/Sbsp/State_MapSbsp.h"
#include "Core/States/Domain/Map/Scen/State_MapScen.h"
#include "Core/States/Domain/Map/Bloc/State_MapBloc.h"
#include "Core/States/Domain/Map/Mach/State_MapMach.h"
#include "Core/States/Domain/Navigation/State_Navigation.h"
#include "Core/Systems/Core_System.h"
#include "Core/Systems/Domain/Core_System_Domain.h"
#include "Core/Systems/Domain/Navigation/Sbsp/System_SbspGeometryBuilder.h"
#include "Core/Systems/Domain/Navigation/Sbsp/System_SbspSeamLinker.h"
#include "Core/Systems/Domain/Navigation/Scen/System_ScenObstacleBuilder.h"
#include "Core/Systems/Domain/Navigation/Bloc/System_BlocObstacleBuilder.h"
#include "Core/Systems/Domain/Navigation/Bloc/System_BlocTeleporterBuilder.h"
#include "Core/Systems/Domain/Navigation/Mach/System_MachDataBuilder.h"
#include "Core/Systems/Domain/Navigation/System_Navigation.h"
#include "Core/Systems/Interface/System_Debug.h"

void System_Navigation::BuildForMap()
{
	const int32_t tagCount =
		static_cast<int32_t>(g_pState->Domain->Map->GetTagsSize());

	std::vector<SbspGeometry> geometries;
	std::vector<const SbspObject*> sbspObjects;

	int32_t sbspCount = 0;
	int32_t scenCount = 0;
	int32_t blocObstacleCount = 0;
	int32_t blocTeleportCount = 0;
	int32_t machCount = 0;

	for (int32_t i = 0; i < tagCount; ++i)
	{
		const Map_TagTableEntry& entry = g_pState->Domain->Map->GetTag(i);
		if (entry.TagGroupIndex < 0) continue;

		const uint32_t magic =
			g_pState->Domain->Map->GetGroupMagic(entry.TagGroupIndex);

		const std::string tagName = g_pState->Domain->Map->GetTagName(i);
		if (tagName.empty()) continue;

		if (magic == MapMagics::k_SbspMagic)
		{
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
				g_pSystem->Domain->SbspGeometryBuilder->BuildGeometry(*sbsp);

			g_pSystem->Debug->Log("[NavigationSystem] INFO:"
				" Built %d clusters, %d portals, %d markers, %d seams.",
				geometry.Clusters.size(), geometry.Portals.size(),
				geometry.Markers.size(), sbsp->StructureSeams.size());

			geometries.push_back(std::move(geometry));
			sbspObjects.push_back(sbsp);
			++sbspCount;
		}
		else if (magic == MapMagics::k_ScenMagic)
		{
			const ScenObject* scen = g_pState->Domain->MapScen->GetScen(tagName);
			if (!scen)
			{
				g_pSystem->Debug->Log("[NavigationSystem] WARNING:"
					" Scen tag found in table but not loaded: ", tagName);
				continue;
			}

			if (g_pSystem->Domain->ScenObstacleBuilder->IsNavigationRelevant(*scen))
			{
				SceneryObstacleData obstacle =
					g_pSystem->Domain->ScenObstacleBuilder->BuildData(*scen);

				g_pState->Domain->Navigation->AddScenObstacle(tagName, std::move(obstacle));
				++scenCount;
			}
		}
		else if (magic == MapMagics::k_BlocMagic)
		{
			const BlocObject* bloc = g_pState->Domain->MapBloc->GetBloc(tagName);
			if (!bloc)
			{
				g_pSystem->Debug->Log("[NavigationSystem] WARNING:"
					" Bloc tag found in table but not loaded: ", tagName);
				continue;
			}

			if (g_pSystem->Domain->BlocObstacleBuilder->IsObstacleRelevant(*bloc))
			{
				CrateObstacleData obstacle =
					g_pSystem->Domain->BlocObstacleBuilder->BuildData(*bloc);

				g_pState->Domain->Navigation->AddBlocObstacle(tagName, std::move(obstacle));
				++blocObstacleCount;
			}

			if (g_pSystem->Domain->BlocTeleporterBuilder->IsTeleporter(*bloc))
			{
				BlocTeleporterData teleporter =
					g_pSystem->Domain->BlocTeleporterBuilder->BuildData(*bloc);

				g_pState->Domain->Navigation->AddBlocTeleporter(tagName, std::move(teleporter));
				++blocTeleportCount;
			}
		}
		else if (magic == MapMagics::k_MachMagic)
		{
			const MachObject* mach = g_pState->Domain->MapMach->GetMach(tagName);
			if (!mach)
			{
				g_pSystem->Debug->Log("[NavigationSystem] WARNING:"
					" Bloc tag found in table but not loaded: ", tagName);
				continue;
			}

			MachineData obstacle =
				g_pSystem->Domain->MachDataBuilder->BuildData(*mach);
			
			g_pState->Domain->Navigation->AddMach(tagName, std::move(obstacle));
			++machCount;
		}
	}

	// Link clusters across BSP boundaries via Structure Seam centroid matching.
	// This mutates CrossLinks inside each geometry's clusters.
	if (sbspCount > 1)
	{
		g_pSystem->Domain->SbspSeamLinker->LinkSeams(geometries, sbspObjects);

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
		g_pState->Domain->Navigation->AddSbspGeometry(geometry);
	}

	g_pSystem->Debug->Log("[NavigationSystem] INFO: Navigation built."
		" Total SBSPs: %d | Scen: %d | Bloc, obstacle: %d |"
		" Bloc, telport: %d | Mach: %d", 
		sbspCount, scenCount, blocObstacleCount, blocTeleportCount, machCount);
}

void System_Navigation::Cleanup()
{
	g_pState->Domain->Navigation->Cleanup();
	g_pSystem->Debug->Log("[NavigationSystem] INFO: Cleanup completed.");
}