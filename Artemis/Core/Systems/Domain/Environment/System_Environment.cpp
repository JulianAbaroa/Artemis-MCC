#include "pch.h"
#include "Core/Common/Map/MapMagics.h"
#include "Core/States/Core_State.h"
#include "Core/States/Domain/Core_State_Domain.h"
#include "Core/States/Domain/Map/State_Map.h"
#include "Core/States/Domain/Map/Coll/State_MapColl.h"
#include "Core/States/Domain/Map/Phmo/State_MapPhmo.h"
#include "Core/States/Domain/Map/Mode/State_MapMode.h"
#include "Core/States/Domain/Map/Scnr/State_MapScnr.h"
#include "Core/States/Domain/Map/Bipd/State_MapBipd.h"
#include "Core/States/Domain/Environment/State_Environment.h"
#include "Core/Systems/Core_System.h"
#include "Core/Systems/Domain/Core_System_Domain.h"
#include "Core/Systems/Domain/Environment/System_Environment.h"
#include "Core/Systems/Domain/Environment/Coll/System_CollGeometryBuilder.h"
#include "Core/Systems/Domain/Environment/Phmo/System_PhmoGeometryBuilder.h"
#include "Core/Systems/Domain/Environment/Mode/System_ModeGeometryBuilder.h"
#include "Core/Systems/Domain/Environment/Scnr/System_ScnrZoneBuilder.h"
#include "Core/Systems/Domain/Environment/Bipd/System_BipdDataBuilder.h"
#include "Core/Systems/Interface/System_Debug.h"

void System_Environment::BuildForMap()
{
	const int32_t tagCount =
		static_cast<int32_t>(g_pState->Domain->Map->GetTagsSize());

	int32_t collCount = 0;
	int32_t phmoCount = 0;
	int32_t modeCount = 0;
	int32_t scnrCount = 0;
	int32_t bipdCount = 0;

	for (int32_t i = 0; i < tagCount; ++i)
	{
		const Map_TagTableEntry& entry = g_pState->Domain->Map->GetTag(i);
		if (entry.TagGroupIndex < 0) continue;

		const uint32_t magic =
			g_pState->Domain->Map->GetGroupMagic(entry.TagGroupIndex);

		const std::string tagName = g_pState->Domain->Map->GetTagName(i);
		if (tagName.empty()) continue;

		if (magic == MapMagics::k_CollMagic)
		{
			const CollObject* coll = g_pState->Domain->MapColl->GetColl(tagName);
			if (!coll)
			{
				g_pSystem->Debug->Log("[EnvironmentSystem] WARNING:"
					" Coll tag found in table but not loaded: ", tagName);
				continue;
			}

			CollGeometry geometry =
				g_pSystem->Domain->CollGeometryBuilder->BuildGeometry(*coll);

			g_pState->Domain->Environment->AddCollGeometry(tagName, std::move(geometry));
			++collCount;
		}
		else if (magic == MapMagics::k_PhmoMagic)
		{
			const PhmoObject* phmo = g_pState->Domain->MapPhmo->GetPhmo(tagName);
			if (!phmo)
			{
				g_pSystem->Debug->Log("[EnvironmentSystem] WARNING:"
					" Phmo tag found in table but not loaded: ", tagName);
				continue;
			}

			PhmoGeometry geometry =
				g_pSystem->Domain->PhmoGeometryBuilder->BuildGeometry(*phmo);

			g_pState->Domain->Environment->AddPhmoGeometry(tagName, std::move(geometry));
			++phmoCount;
		}
		else if (magic == MapMagics::k_ModeMagic)
		{
			const ModeObject* mode = g_pState->Domain->MapMode->GetMode(tagName);
			if (!mode)
			{
				g_pSystem->Debug->Log("[EnvironmentSystem] WARNING:"
					" Mode tag found in table but not loaded: ", tagName);
				continue;
			}

			ModeGeometry geometry =
				g_pSystem->Domain->ModeGeometryBuilder->BuildGeometry(*mode);

			g_pState->Domain->Environment->AddModeGeometry(tagName, std::move(geometry));
			++modeCount;
		}
		else if (magic == MapMagics::k_ScnrMagic)
		{
			const ScnrObject* scnr = g_pState->Domain->MapScnr->GetScnr(tagName);
			if (!scnr)
			{
				g_pSystem->Debug->Log("[EnvironmentSystem] WARNING:"
					" Scnr tag found in table but not loaded: ", tagName);
				continue;
			}

			ScnrMapZones zones =
				g_pSystem->Domain->ScnrZoneBuilder->BuildZone(*scnr);

			g_pState->Domain->Environment->SetMapZones(std::move(zones));
			++scnrCount;
		}
		else if (magic == MapMagics::k_BipdMagic)
		{
			const BipdObject* bipd = g_pState->Domain->MapBipd->GetBipd(tagName);
			if (!bipd)
			{
				g_pSystem->Debug->Log("[EnvironmentSystem] WARNING:"
					" Bipd tag found in table but not loaded: ", tagName);
				continue;
			}

			BipdPhysicsData data =
				g_pSystem->Domain->BipdDataBuilder->BuildData(*bipd);

			g_pState->Domain->Environment->AddBipdData(tagName, std::move(data));
			++bipdCount;
		}
	}

	g_pSystem->Debug->Log("[EnvironmentSystem] INFO:"
		" Environment built. Coll: %d, Phmo: %d, Mode: %d, Scnr: %d"
		" Bipd: %d", collCount, phmoCount, modeCount, scnrCount, bipdCount);
}

void System_Environment::Cleanup()
{
	g_pState->Domain->Environment->Cleanup();
	g_pSystem->Debug->Log("[EnvironmentSystem] INFO: Cleanup completed.");
}