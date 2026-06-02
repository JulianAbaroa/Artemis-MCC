#include "pch.h"

#include "Core_System.h"

#include "Core/States/Core_State.h"

#include "MapReader/System_MapReader.h"
#include "MapReader/TagGroup/System_TagGroupReader.h"
#include "MapReader/Mesh/System_MeshReader.h"

#include "Tables/Object/System_ObjectTable.h"
#include "Tables/Player/System_PlayerTable.h"
#include "Tables/Interaction/System_InteractionTable.h"

#include "Filtered/Graph/Object/System_ObjectGraph.h"
#include "Filtered/Graph/Player/System_PlayerGraph.h"
#include "Filtered/Classifier/System_Classifier.h"

#include "Domains/Navigation/System_Navigation.h"
#include "Domains/Navigation/Sbsp/System_SbspBuilder.h"
#include "Domains/Navigation/Sbsp/System_SbspSeamLinker.h"
#include "Domains/Navigation/Scnr/System_ScnrBuilder.h"

#include "Domains/Environment/System_Environment.h"
#include "Domains/Environment/Coll/System_CollBuilder.h"
#include "Domains/Environment/Mode/System_ModeBuilder.h"
#include "Domains/Environment/Phmo/System_PhmoBuilder.h"

#include "Domains/Interactable/System_Interactable.h"
#include "Domains/Interactable/Proj/System_ProjBuilder.h"
#include "Domains/Interactable/Vehi/System_VehiBuilder.h"
#include "Domains/Interactable/Weap/System_WeapBuilder.h"

#include "Memory/AOBScanner/System_AOBScanner.h"
#include "Memory/MemoryReader/System_MemoryReader.h"
#include "Memory/MemoryScanner/System_MemoryScanner.h"

#include "Input/System_Input.h"
#include "Lifecycle/System_Lifecycle.h"
#include "Render/System_Render.h"
#include "Settings/System_Settings.h"
#include "Logs/System_Logs.h"

Core_System::Core_System() = default;
Core_System::~Core_System() = default;

void Core_System::Initialize(Core_State& state)
{
	Logs = std::make_unique<System_Logs>(
		System_Logs_Dependencies {
			.State_Settings = *state.Settings,
			.State_Logs = *state.Logs,
		});

	this->InitMapReaders(state);
	this->InitTables(state);
	this->InitFiltered(state);
	this->InitNavigation(state);
	this->InitEnvironment(state);
	this->InitInteractable(state);
	this->InitMemory(state);

	Input = std::make_unique<System_Input>(
		System_Input_Dependencies {
			.State_Input = *state.Input,
			.System_Logs = *Logs,
		});

	Lifecycle = std::make_unique<System_Lifecycle>(
		System_Lifecycle_Dependencies {
			.State_Lifecycle = *state.Lifecycle,
			.System_Logs = *Logs,
		});

	Render = std::make_unique<System_Render>(
		System_Render_Dependencies {
			.State_Render = *state.Render,
			.State_Settings = *state.Settings,
			.System_Logs = *Logs,
		});

	Settings = std::make_unique<System_Settings>(
		System_Settings_Dependencies {
			.State_Settings = *state.Settings,
			.System_Logs = *Logs,
		});
}

void Core_System::InitMapReaders(Core_State& state)
{
	MapReader = std::make_unique<System_MapReader>(
		System_MapReader_Dependencies{
			.State_Map = *state.MapReader,
			.System_Logs = *Logs,
		});

	TagGroupReader = std::make_unique<System_TagGroupReader>(
		System_TagGroupReader_Dependencies{
			.State_Map = *state.MapReader,
			.State_MapPhmo = *state.MapPhmo,
			.State_MapColl = *state.MapColl,
			.State_MapMode = *state.MapMode,
			.State_MapJmad = *state.MapJmad,
			.State_MapVehi = *state.MapVehi,
			.State_MapBloc = *state.MapBloc,
			.State_MapSbsp = *state.MapSbsp,
			.State_MapWeap = *state.MapWeap,
			.State_MapProj = *state.MapProj,
			.State_MapBipd = *state.MapBipd,
			.State_MapEqip = *state.MapEqip,
			.State_MapScen = *state.MapScen,
			.State_MapScnr = *state.MapScnr,
			.State_MapCtrl = *state.MapCtrl,
			.State_MapMach = *state.MapMach,
			.State_MapZone = *state.MapZone,
			.State_MapPlay = *state.MapPlay,
			.State_MapSldt = *state.MapSldt,
			.State_MapLbsp = *state.MapLbsp,
			.System_Header = *MapReader,
			.System_Logs = *Logs,
		});

	MeshReader = std::make_unique<System_MeshReader>(
		System_MeshReader_Dependencies{
			.State_MapReader = *state.MapReader,
			.State_MapScnr = *state.MapScnr,
			.State_MapSbsp = *state.MapSbsp,
			.State_MapLbsp = *state.MapLbsp,
			.State_MapPlay= *state.MapPlay,
			.State_MapZone = *state.MapZone,
			.System_MapReader = *MapReader,
			.System_Logs = *Logs,
		});
}

void Core_System::InitTables(Core_State& state)
{
	ObjectTable = std::make_unique<System_ObjectTable>(
		System_ObjectTable_Dependencies{
			.State_ObjectTable = *state.ObjectTable,
			.State_MapPhmo = *state.MapPhmo,
			.State_MapColl = *state.MapColl,
			.State_MapMode = *state.MapMode,
			.State_MapJmad = *state.MapJmad,
			.State_MapVehi = *state.MapVehi,
			.State_MapBloc = *state.MapBloc,
			.State_MapSbsp = *state.MapSbsp,
			.State_MapWeap = *state.MapWeap,
			.State_MapProj = *state.MapProj,
			.State_MapBipd = *state.MapBipd,
			.State_MapEqip = *state.MapEqip,
			.State_MapScen = *state.MapScen,
			.State_MapScnr = *state.MapScnr,
			.State_MapCtrl = *state.MapCtrl,
			.State_MapMach = *state.MapMach,
			.System_MapReader = *MapReader,
			.System_MemoryReader = *MemoryReader,
			.System_Logs = *Logs,
		});

	PlayerTable = std::make_unique<System_PlayerTable>(
		System_PlayerTable_Dependencies{
			.State_PlayerTable = *state.PlayerTable,
			.System_MemoryReader = *MemoryReader,
			.System_Logs = *Logs,
		});

	InteractionTable = std::make_unique<System_InteractionTable>(
		System_InteractionTable_Dependencies{
			.State_InteractionTable = *state.InteractionTable,
			.System_MemoryReader = *MemoryReader,
			.System_Logs = *Logs,
		});
}

void Core_System::InitFiltered(Core_State& state)
{
	ObjectGraph = std::make_unique<System_ObjectGraph>(
		System_ObjectGraph_Dependencies{
			.State_ObjectTable = *state.ObjectTable,
			.State_ObjectGraph = *state.ObjectGraph,
			.System_Logs = *Logs,
		});

	PlayerGraph = std::make_unique<System_PlayerGraph>(
		System_PlayerGraph_Dependencies{
			.State_ObjectTable = *state.ObjectTable,
			.State_PlayerTable = *state.PlayerTable,
			.State_Classification = *state.Classifier,
			.State_ObjectGraph = *state.ObjectGraph,
			.State_PlayerGraph = *state.PlayerGraph,
			.System_Logs = *Logs,
		});

	Classifier = std::make_unique<System_Classifier>(
		System_Classifier_Dependencies{
			.State_ObjectTable = *state.ObjectTable,
			.State_PlayerTable = *state.PlayerTable,
			.State_Classification = *state.Classifier,
			.State_Navigation = *state.Navigation,
			.State_Environment = *state.Environment,
			.State_Interactable = *state.Interactable,
			.System_Logs= *Logs,
		});
}

void Core_System::InitNavigation(Core_State& state)
{
	SbspBuilder = std::make_unique<System_SbspBuilder>(
		SbspBuilder_Dependencies{
			.State_MapZone = *state.MapZone,
			.State_MapPlay = *state.MapPlay,
			.State_MapLbsp = *state.MapLbsp,
			.System_Header = *MapReader,
			.System_MeshReader = *MeshReader,
			.System_Logs = *Logs,
		});

	SbspSeamLinker = std::make_unique<System_SbspSeamLinker>();
	ScnrBuilder = std::make_unique<System_ScnrBuilder>();

	Navigation = std::make_unique<System_Navigation>(
		System_Navigation_Dependencies{
			.State_Map = *state.MapReader,
			.State_MapSbsp = *state.MapSbsp,
			.State_MapScen = *state.MapScen,
			.State_MapBloc = *state.MapBloc,
			.State_MapMach = *state.MapMach,
			.State_MapScnr = *state.MapScnr,
			.State_ObjectTable = *state.ObjectTable,
			.State_Classification = *state.Classifier,
			.State_Navigation = *state.Navigation,
			.System_SbspBuilder = *SbspBuilder,
			.System_SbspSeamLinker = *SbspSeamLinker,
			.System_ScnrBuilder = *ScnrBuilder,
			.System_MeshReader = *MeshReader,
			.System_Logs = *Logs,
		});
}

void Core_System::InitEnvironment(Core_State& state)
{
	CollBuilder = std::make_unique<System_CollBuilder>();
	ModeBuilder = std::make_unique<System_ModeBuilder>();
	PhmoBuilder = std::make_unique<System_PhmoBuilder>();

	Environment = std::make_unique<System_Environment>(
		System_Environment_Dependencies{
			.State_Map = *state.MapReader,
			.State_MapColl = *state.MapColl,
			.State_MapPhmo = *state.MapPhmo,
			.State_MapMode = *state.MapMode,
			.State_MapScen = *state.MapScen,
			.State_ObjectTable = *state.ObjectTable,
			.State_Classification = *state.Classifier,
			.State_Environment = *state.Environment,
			.System_CollBuilder = *CollBuilder,
			.System_ModeBuilder = *ModeBuilder,
			.System_PhmoBuilder = *PhmoBuilder,
			.System_Logs = *Logs,
		});
}

void Core_System::InitInteractable(Core_State& state)
{
	ProjBuilder = std::make_unique<System_ProjBuilder>();
	VehiBuilder = std::make_unique<System_VehiBuilder>();
	WeapBuilder = std::make_unique<System_WeapBuilder>();

	Interactable = std::make_unique<System_Interactable>(
		System_Interactable_Dependencies{
			.State_Map = *state.MapReader,
			.State_MapVehi = *state.MapVehi,
			.State_MapEqip = *state.MapEqip,
			.State_MapWeap = *state.MapWeap,
			.State_MapProj = *state.MapProj,
			.State_MapCtrl = *state.MapCtrl,
			.State_ObjectTable = *state.ObjectTable,
			.State_PlayerTable = *state.PlayerTable,
			.State_InteractionTable = *state.InteractionTable,
			.State_Classification = *state.Classifier,
			.State_ObjectGraph = *state.ObjectGraph,
			.State_PlayerGraph = *state.PlayerGraph,
			.State_Environment = *state.Environment,
			.State_Interactable = *state.Interactable,
			.System_Classifier = *Classifier,
			.System_ProjBuilder = *ProjBuilder,
			.System_VehiBuilder = *VehiBuilder,
			.System_WeapBuilder = *WeapBuilder,
			.System_Logs = *Logs,
		});
}

void Core_System::InitMemory(Core_State& state) 
{
	AOBScanner = std::make_unique<System_AOBScanner>(*Logs);

	MemoryReader = std::make_unique<System_MemoryReader>();

	MemoryScanner = std::make_unique<System_MemoryScanner>(
		System_MemoryScanner_Dependencies{
			.State_MemoryScanner = *state.Memory,
			.System_Logs = *Logs,
		});
}

void Core_System::Deinitialize()
{
	this->DeinitMapReaders();
	this->DeinitTables();
	this->DeinitFiltered();
	this->DeinitNavigation();
	this->DeinitEnvironment();
	this->DeinitInteractable();
	this->DeinitMemory();

	Input.reset();
	Lifecycle.reset();
	Render.reset();
	Settings.reset();
	Logs.reset();
}

void Core_System::DeinitMapReaders()
{
	MapReader.reset();
	TagGroupReader.reset();
	MeshReader.reset();
}

void Core_System::DeinitTables()
{
	ObjectTable.reset();
	PlayerTable.reset();
	InteractionTable.reset();
}

void Core_System::DeinitFiltered()
{
	ObjectGraph.reset();
	PlayerGraph.reset();
	Classifier.reset();
}

void Core_System::DeinitNavigation()
{
	Navigation.reset();
	SbspBuilder.reset();
	SbspSeamLinker.reset();
	ScnrBuilder.reset();
}

void Core_System::DeinitEnvironment()
{
	CollBuilder.reset();
	ModeBuilder.reset();
	PhmoBuilder.reset();
	Environment.reset();
}

void Core_System::DeinitInteractable()
{
	Interactable.reset();
	ProjBuilder.reset();
	VehiBuilder.reset();
	WeapBuilder.reset();
}

void Core_System::DeinitMemory() 
{
	AOBScanner.reset();
	MemoryReader.reset();
	MemoryScanner.reset();
}