#include "pch.h"

#include "Thread_AI.h"

#include "Core/States/MapReader/State_MapReader.h"
#include "Core/States/Tables/Player/State_PlayerTable.h"
#include "Core/States/Domains/Navigation/State_Navigation.h"
#include "Core/States/Lifecycle/State_Lifecycle.h"

#include "Core/Systems/MapReader/TagGroup/System_TagGroupReader.h"
#include "Core/Systems/Tables/Object/System_ObjectTable.h"
#include "Core/Systems/Tables/Player/System_PlayerTable.h"
#include "Core/Systems/Tables/Interaction/System_InteractionTable.h"
#include "Core/Systems/Filtered/Classifier/System_Classifier.h"
#include "Core/Systems/Filtered/Graph/Object/System_ObjectGraph.h"
#include "Core/Systems/Filtered/Graph/Player/System_PlayerGraph.h"
#include "Core/Systems/Domains/Navigation/System_Navigation.h"
#include "Core/Systems/Domains/Environment/System_Environment.h"
#include "Core/Systems/Domains/Interactable/System_Interactable.h"
#include "Core/Systems/Logs/System_Logs.h"

#include <chrono>

using namespace std::chrono_literals;

void Thread_AI::Run()
{
	m_Deps.System_Logs.Log("[AIThread] INFO: Started.");

	while (m_Deps.State_Lifecycle.IsRunning())
	{
		if (m_Deps.State_MapReader.IsLoaded())
		{
			if (!m_WasLoaded)
			{
				m_Deps.System_TagGroupReader.LoadForMap();
				m_Deps.System_Navigation.BuildForMap();
				m_Deps.System_Environment.BuildForMap();
				m_Deps.System_Interactable.BuildForMap();
				m_WasLoaded = true;
			}

			// Update tables.
			m_Deps.System_ObjectTable.UpdateObjectTable();
			m_Deps.System_PlayerTable.UpdatePlayerTable();
			m_Deps.System_InteractionTable.UpdateInteractionTable();

			// Classify objects.
			m_Deps.System_Classifier.UpdateClassification();

			// Update graphs.
			m_Deps.System_ObjectGraph.UpdateGraph();
			m_Deps.System_PlayerGraph.UpdateGraph();

			if (!m_Deps.State_Navigation.HasSbspGeometry()) continue;

			// TODO: Agents, Self, Observable.

			// Update domains.
			m_Deps.System_Navigation.UpdateNavigation();
			m_Deps.System_Environment.UpdateEnvironment();
			m_Deps.System_Interactable.UpdateInteractables();
		}
		else m_WasLoaded = false;

		std::this_thread::sleep_for(16ms);
	}

	m_Deps.System_Logs.Log("[AIThread] INFO: Stopped.");
}