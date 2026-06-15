#include "pch.h"

#include "Core_System.h"

#include "Core/States/Core_State.h"

#include "Sources/MapReader/System_MapReader.h"
#include "Sources/MapReader/TagGroup/System_TagGroupReader.h"
#include "Sources/MapReader/Geometry/System_GeometryReader.h"
#include "Sources/Tables/Object/System_ObjectTable.h"
#include "Sources/Tables/Player/System_PlayerTable.h"
#include "Sources/Tables/Interaction/System_InteractionTable.h"
#include "Structure/Graph/Object/System_ObjectGraph.h"
#include "Structure/Graph/Player/System_PlayerGraph.h"
#include "Structure/Classifier/System_Classifier.h"
#include "Sources/Static/Stats/System_StatsBuilder.h"
#include "Sources/Static/Stats/Proj/System_ProjBuilder.h"
#include "Sources/Static/Stats/Vehi/System_VehiBuilder.h"
#include "Sources/Static/Stats/Weap/System_WeapBuilder.h"
#include "Sources/Static/World/System_WorldBuilder.h"
#include "Sources/Static/World/Coll/System_CollBuilder.h"
#include "Sources/Static/World/Mode/System_ModeBuilder.h"
#include "Sources/Static/World/Phmo/System_PhmoBuilder.h"
#include "Sources/Static/World/Sbsp/System_SbspBuilder.h"
#include "Sources/Static/World/Sbsp/System_SbspSeamLinker.h"
#include "Sources/Static/Vitality/System_VitalityBuilder.h"
#include "Environment/Collidables/System_Collidables.h"
#include "Environment/Vitality/System_Vitality.h"
#include "Environment/Fixtures/System_Fixtures.h"
#include "Egocentric/Self/System_Self.h"
#include "Egocentric/Affordances/System_Affordances.h"
#include "Tick/System_Tick.h"
#include "Other/Input/System_Input.h"
#include "Other/Lifecycle/System_Lifecycle.h"
#include "Other/Logs/System_Logs.h"
#include "Other/Memory/AOB/System_AOBScanner.h"
#include "Other/Memory/Reader/System_MemoryReader.h"
#include "Other/Memory/Scanner/System_MemoryScanner.h"
#include "Other/Render/System_Render.h"
#include "Other/Render/Map/System_MapRenderer.h"
#include "Other/Settings/System_Settings.h"
#include "Other/Telemetry/System_Telemetry.h"

Core_System::Core_System() = default;
Core_System::~Core_System() = default;

void Core_System::Initialize(Core_State& state)
{
	Logs = std::make_unique<System_Logs>(
		Sys_Logs_Deps {
			.State_Settings = *state.Settings,
			.State_Logs = *state.Logs,
		});

	Input = std::make_unique<System_Input>(
		Sys_Input_Deps{
			.State_Input = *state.Input,
			.System_Logs = *Logs,
		});

	Lifecycle = std::make_unique<System_Lifecycle>(
		Sys_Lifecycle_Deps{
			.State_Lifecycle = *state.Lifecycle,
			.System_Logs = *Logs,
		});

	AOBScanner = std::make_unique<System_AOBScanner>(
		Sys_AOBScanner_Deps{
			.System_Logs = *Logs,
		});

	MemoryReader = std::make_unique<System_MemoryReader>();

	MemoryScanner = std::make_unique<System_MemoryScanner>(
		Sys_MemoryScanner_Deps{
			.State_MemoryScanner = *state.Memory,
			.System_Logs = *Logs,
		});

	Render = std::make_unique<System_Render>(
		Sys_Render_Deps{
			.State_Render = *state.Render,
			.State_FlyCamera = *state.FlyCamera,
			.State_Settings = *state.Settings,
			.System_Logs = *Logs,
		});

	MapRenderer = std::make_unique<System_MapRenderer>(
		Sys_MapRenderer_Deps{
			.State_WorldBuilder = *state.WorldBuilder,
			.State_Render = *state.Render,
			.State_FlyCamera = *state.FlyCamera,
			.State_Selection = *state.Selection,
			.System_Logs = *Logs,
		});

	Settings = std::make_unique<System_Settings>(
		Sys_Settings_Deps{
			.State_Settings = *state.Settings,
			.System_Logs = *Logs,
		});

	Telemetry = std::make_unique<System_Telemetry>(
		Sys_Telemetry_Deps{
			.State_Telemetry = *state.Telemetry,
		});

	Tick = std::make_unique<System_Tick>(
		Sys_Tick_Deps{
			.State_Tick = *state.Tick,
			.State_ObjectTable = *state.ObjectTable,
			.State_PlayerTable = *state.PlayerTable,
			.State_InteractionTable = *state.InteractionTable,
			.State_Classifier = *state.Classifier,
			.State_ObjectGraph = *state.ObjectGraph,
			.State_PlayerGraph = *state.PlayerGraph,
			.State_Collidables = *state.Collidables,
			.State_Fixtures = *state.Fixtures,
			.State_Vitality = *state.Vitality,
			.State_Self = *state.Self,
			.State_Affordances = *state.Affordances,
		});

	this->InitSources(state);
	this->InitStructure(state);
	this->InitEnvironment(state);
	this->InitEgocentric(state);
}

void Core_System::InitSources(Core_State& state)
{
	MapReader = std::make_unique<System_MapReader>(
		Sys_MapReader_Deps{
			.State_MapReader = *state.MapReader,
			.System_Logs = *Logs,
		});

	TagGroupReader = std::make_unique<System_TagGroupReader>(
		Sys_TagGroupReader_Deps{
			.State_MapReader = *state.MapReader,
			.State_MapBipd = *state.MapBipd,
			.State_MapBloc = *state.MapBloc,
			.State_MapColl = *state.MapColl,
			.State_MapCtrl = *state.MapCtrl,
			.State_MapEqip = *state.MapEqip,
			.State_MapHlmt = *state.MapHlmt,
			.State_MapLbsp = *state.MapLbsp,
			.State_MapMach = *state.MapMach,
			.State_MapMode = *state.MapMode,
			.State_MapPhmo = *state.MapPhmo,
			.State_MapPlay = *state.MapPlay,
			.State_MapProj = *state.MapProj,
			.State_MapSbsp = *state.MapSbsp,
			.State_MapScen = *state.MapScen,
			.State_MapScnr = *state.MapScnr,
			.State_MapSldt = *state.MapSldt,
			.State_MapVehi = *state.MapVehi,
			.State_MapWeap = *state.MapWeap,
			.State_MapZone = *state.MapZone,
			.System_MapReader = *MapReader,
			.System_Logs = *Logs,
		});

	GeometryReader = std::make_unique<System_GeometryReader>(
		Sys_MeshReader_Deps{
			.State_MapReader = *state.MapReader,
			.State_MapScnr = *state.MapScnr,
			.State_MapSbsp = *state.MapSbsp,
			.State_MapLbsp = *state.MapLbsp,
			.State_MapPlay= *state.MapPlay,
			.State_MapZone = *state.MapZone,
			.System_MapReader = *MapReader,
			.System_Logs = *Logs,
		});

	ObjectTable = std::make_unique<System_ObjectTable>(
		Sys_ObjectTable_Deps{
			.State_MapBipd = *state.MapBipd,
			.State_MapBloc = *state.MapBloc,
			.State_MapColl = *state.MapColl,
			.State_MapCtrl = *state.MapCtrl,
			.State_MapEqip = *state.MapEqip,
			.State_MapHlmt = *state.MapHlmt,
			.State_MapLbsp = *state.MapLbsp,
			.State_MapMach = *state.MapMach,
			.State_MapMode = *state.MapMode,
			.State_MapPhmo = *state.MapPhmo,
			.State_MapPlay = *state.MapPlay,
			.State_MapProj = *state.MapProj,
			.State_MapSbsp = *state.MapSbsp,
			.State_MapScen = *state.MapScen,
			.State_MapScnr = *state.MapScnr,
			.State_MapSldt = *state.MapSldt,
			.State_MapVehi = *state.MapVehi,
			.State_MapWeap = *state.MapWeap,
			.State_MapZone = *state.MapZone,
			.State_ObjectTable = *state.ObjectTable,
			.State_BoneOffsets = *state.BoneOffsets,
			.State_BoneMatrices = *state.BoneMatrices,
			.State_DamageSections = *state.DamageSections,
			.State_WorldBuilder = *state.WorldBuilder,
			.System_MapReader = *MapReader,
			.System_MemoryReader = *MemoryReader,
			.System_Logs = *Logs,
		});

	PlayerTable = std::make_unique<System_PlayerTable>(
		Sys_PlayerTable_Deps{
			.State_PlayerTable = *state.PlayerTable,
			.System_MemoryReader = *MemoryReader,
			.System_Logs = *Logs,
		});

	InteractionTable = std::make_unique<System_InteractionTable>(
		Sys_InteractionTable_Deps{
			.State_InteractionTable = *state.InteractionTable,
			.System_MemoryReader = *MemoryReader,
			.System_Logs = *Logs,
		});

	WeapBuilder = std::make_unique<System_WeapBuilder>(
		Sys_WeapBuilder_Deps{
			.System_Logs = *Logs,
		});

	VehiBuilder = std::make_unique<System_VehiBuilder>(
		Sys_VehiBuilder_Deps{
			.System_Logs = *Logs,
		});

	ProjBuilder = std::make_unique<System_ProjBuilder>(
		Sys_ProjBuilder_Deps{
			.System_Logs = *Logs,
		});

	StatsBuilder = std::make_unique<System_StatsBuilder>(
		Sys_StatsBuilder_Deps{
			.State_MapReader = *state.MapReader,
			.State_MapProj = *state.MapProj,
			.State_MapVehi = *state.MapVehi,
			.State_MapWeap = *state.MapWeap,
			.State_StatsBuilder = *state.StatsBuilder,
			.System_ProjBuilder = *ProjBuilder,
			.System_VehiBuilder = *VehiBuilder,
			.System_WeapBuilder = *WeapBuilder,
			.System_Logs = *Logs,
		});

	CollBuilder = std::make_unique<System_CollBuilder>(
		Sys_CollBuilder_Deps{
			.System_Logs = *Logs,
		});

	ModeBuilder = std::make_unique<System_ModeBuilder>(
		Sys_ModeBuilder_Deps{
			.System_Logs = *Logs,
		});

	PhmoBuilder = std::make_unique<System_PhmoBuilder>(
		Sys_PhmoBuilder_Deps{
			.System_Logs = *Logs,
		});

	SbspSeamLinker = std::make_unique<System_SbspSeamLinker>(
		Sys_SbspSeamLinker_Deps{
			.System_Logs = *Logs,
		});

	SbspBuilder = std::make_unique<System_SbspBuilder>(
		Sys_SbspBuilder_Deps{
			.State_MapZone = *state.MapZone,
			.State_MapPlay = *state.MapPlay,
			.State_MapLbsp = *state.MapLbsp,
			.System_Header = *MapReader,
			.System_GeometryReader = *GeometryReader,
			.System_Logs = *Logs,
		});

	WorldBuilder = std::make_unique<System_WorldBuilder>(
		Sys_WorldBuilder_Deps{
			.State_MapReader = *state.MapReader,
			.State_MapBipd = *state.MapBipd,
			.State_MapBloc = *state.MapBloc,
			.State_MapColl = *state.MapColl,
			.State_MapCtrl = *state.MapCtrl,
			.State_MapEqip = *state.MapEqip,
			.State_MapHlmt = *state.MapHlmt,
			.State_MapMach = *state.MapMach,
			.State_MapMode = *state.MapMode,
			.State_MapPhmo = *state.MapPhmo,
			.State_MapSbsp = *state.MapSbsp,
			.State_MapScen = *state.MapScen,
			.State_MapVehi = *state.MapVehi,
			.State_MapWeap = *state.MapWeap,
			.State_WorldBuilder = *state.WorldBuilder,
			.System_MapReader = *MapReader,
			.System_SbspSeamLinker = *SbspSeamLinker,
			.System_SbspBuilder = *SbspBuilder,
			.System_CollBuilder = *CollBuilder,
			.System_PhmoBuilder = *PhmoBuilder,
			.System_ModeBuilder = *ModeBuilder,
			.System_GeometryReader = *GeometryReader,
			.System_Logs = *Logs,
		});

	VitalityBuilder = std::make_unique<System_VitalityBuilder>(
		Sys_VitalityBuilder_Deps{
			.State_MapColl = *state.MapColl,
			.State_MapHlmt = *state.MapHlmt,
			.State_VitalityBuilder = *state.VitalityBuilder,
			.System_Logs = *Logs,
		});
}

void Core_System::InitStructure(Core_State& state)
{
	ObjectGraph = std::make_unique<System_ObjectGraph>(
		Sys_ObjectGraph_Deps{
			.State_ObjectTable = *state.ObjectTable,
			.State_ObjectGraph = *state.ObjectGraph,
			.System_Logs = *Logs,
		});

	PlayerGraph = std::make_unique<System_PlayerGraph>(
		Sys_PlayerGraph_Deps{
			.State_ObjectTable = *state.ObjectTable,
			.State_PlayerTable = *state.PlayerTable,
			.State_Classifier = *state.Classifier,
			.State_ObjectGraph = *state.ObjectGraph,
			.State_PlayerGraph = *state.PlayerGraph,
			.System_Logs = *Logs,
		});

	Classifier = std::make_unique<System_Classifier>(
		Sys_Classifier_Deps{
			.State_ObjectTable = *state.ObjectTable,
			.State_PlayerTable = *state.PlayerTable,
			.State_Classifier = *state.Classifier,
			.System_Logs= *Logs,
		});
}

void Core_System::InitEnvironment(Core_State& state) 
{
	Collidables = std::make_unique<System_Collidables>(
		Sys_Collidables_Deps{
			.State_MapHlmt = *state.MapHlmt,
			.State_ObjectTable = *state.ObjectTable,
			.State_BoneMatrices = *state.BoneMatrices,
			.State_DamageSections = *state.DamageSections,
			.State_Classifier = *state.Classifier,
			.State_ObjectGraph = *state.ObjectGraph,
			.State_WorldBuilder = *state.WorldBuilder,
			.State_Collidables = *state.Collidables,
			.System_Logs = *Logs,
		});

	Vitality= std::make_unique<System_Vitality>(
		Sys_Vitality_Deps{
			.State_Classifier = *state.Classifier,
			.State_ObjectTable = *state.ObjectTable,
			.State_DamageSections = *state.DamageSections,
			.State_VitalityBuilder = *state.VitalityBuilder,
			.State_Vitality = *state.Vitality,
			.System_Logs = *Logs,
		});

	Fixtures = std::make_unique<System_Fixtures>(
		Sys_Fixtures_Deps{
			.State_ObjectTable = *state.ObjectTable,
			.State_Classifier = *state.Classifier,
			.State_Fixtures = *state.Fixtures,
			.System_Logs = *Logs,
		});
}

void Core_System::InitEgocentric(Core_State& state)
{
	Self = std::make_unique<System_Self>(
		Sys_Self_Deps{
			.State_PlayerTable = *state.PlayerTable,
			.State_Self = *state.Self,
			.System_Logs = *Logs,
		});

	Affordances = std::make_unique<System_Affordances>(
		Sys_Affordances_Deps{
			.State_ObjectTable = *state.ObjectTable,
			.State_InteractionTable = *state.InteractionTable,
			.State_Self = *state.Self,
			.State_Classifier = *state.Classifier,
			.State_ObjectGraph = *state.ObjectGraph,
			.State_PlayerGraph = *state.PlayerGraph,
			.State_Affordances = *state.Affordances,
			.System_Logs = *Logs,
		});
}

void Core_System::Deinitialize()
{
	Logs.reset();

	this->DeinitSources();
	this->DeinitStructure();
	this->DeinitEnvironment();
	this->DeinitEgocentric();

	Tick.reset();

	Input.reset();
	Telemetry.reset();
	Lifecycle.reset();
	Render.reset();
	MapRenderer.reset();
	Settings.reset();
	AOBScanner.reset();
	MemoryReader.reset();
	MemoryScanner.reset();
}

void Core_System::DeinitSources()
{
	MapReader.reset();
	TagGroupReader.reset();
	GeometryReader.reset();
	ObjectTable.reset();
	PlayerTable.reset();
	InteractionTable.reset();
	StatsBuilder.reset();
	WeapBuilder.reset();
	VehiBuilder.reset();
	ProjBuilder.reset();
	WorldBuilder.reset();
	CollBuilder.reset();
	ModeBuilder.reset();
	PhmoBuilder.reset();
	SbspBuilder.reset();
	SbspSeamLinker.reset();
	VitalityBuilder.reset();
}

void Core_System::DeinitStructure()
{
	ObjectGraph.reset();
	PlayerGraph.reset();
	Classifier.reset();
}

void Core_System::DeinitEnvironment() 
{
	Collidables.reset();
	Vitality.reset();
	Fixtures.reset();
}

void Core_System::DeinitEgocentric()
{
	Self.reset();
	Affordances.reset();
}