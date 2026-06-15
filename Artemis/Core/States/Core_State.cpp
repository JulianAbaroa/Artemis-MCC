#include "pch.h"

#include "Core_State.h"

#include "Sources/MapReader/State_MapReader.h"
#include "Sources/MapReader/Tags/State_MapBipd.h" 
#include "Sources/MapReader/Tags/State_MapBloc.h"
#include "Sources/MapReader/Tags/State_MapColl.h"
#include "Sources/MapReader/Tags/State_MapCtrl.h"
#include "Sources/MapReader/Tags/State_MapEqip.h"
#include "Sources/MapReader/Tags/State_MapLbsp.h"
#include "Sources/MapReader/Tags/State_MapMach.h"
#include "Sources/MapReader/Tags/State_MapMode.h"
#include "Sources/MapReader/Tags/State_MapHlmt.h"
#include "Sources/MapReader/Tags/State_MapPhmo.h"
#include "Sources/MapReader/Tags/State_MapPlay.h"
#include "Sources/MapReader/Tags/State_MapProj.h"
#include "Sources/MapReader/Tags/State_MapSbsp.h"
#include "Sources/MapReader/Tags/State_MapSbsp.h"
#include "Sources/MapReader/Tags/State_MapScen.h"
#include "Sources/MapReader/Tags/State_MapScnr.h"
#include "Sources/MapReader/Tags/State_MapSldt.h"
#include "Sources/MapReader/Tags/State_MapVehi.h"
#include "Sources/MapReader/Tags/State_MapWeap.h"
#include "Sources/MapReader/Tags/State_MapZone.h"
#include "Sources/Tables/Object/State_ObjectTable.h"
#include "Sources/Tables/Object/BoneMatrix/State_BoneOffsets.h"
#include "Sources/Tables/Object/BoneMatrix/State_BoneMatrices.h"
#include "Sources/Tables/Object/DamageSection/State_DamageSections.h"
#include "Sources/Tables/Player/State_PlayerTable.h"
#include "Sources/Tables/Interaction/State_InteractionTable.h"
#include "Sources/Static/World/State_WorldBuilder.h"
#include "Sources/Static/Vitality/State_VitalityBuilder.h"
#include "Sources/Static/Stats/State_StatsBuilder.h"
#include "Structure/Graph/Object/State_ObjectGraph.h"
#include "Structure/Graph/Player/State_PlayerGraph.h"
#include "Structure/Classifier/State_Classifier.h"
#include "Environment/Collidables/State_Collidables.h"
#include "Environment/Vitality/State_Vitality.h"
#include "Environment/Fixtures/State_Fixtures.h"
#include "Egocentric/Self/State_Self.h"
#include "Egocentric/Affordances/State_Affordances.h"
#include "Tick/State_Tick.h"
#include "Other/Input/State_Input.h"
#include "Other/Memory/State_MemoryScanner.h"
#include "Other/Lifecycle/State_Lifecycle.h"
#include "Other/Telemetry/State_Telemetry.h"
#include "Other/Render/State_Render.h"
#include "Other/Render/State_FlyCamera.h"
#include "Other/Render/State_Selection.h"
#include "Other/Render/State_OverlayMode.h"
#include "Other/Settings/State_Settings.h"
#include "Other/Logs/State_Logs.h"

Core_State::Core_State() = default;
Core_State::~Core_State() = default;

void Core_State::Initialize()
{
	this->InitSources();
	this->InitStructure();
	this->InitEnvironment();
	this->InitEgocentric();

	Tick = std::make_unique<State_Tick>();

	Input = std::make_unique<State_Input>();
	Memory = std::make_unique<State_MemoryScanner>();
	Telemetry = std::make_unique<State_Telemetry>();
	Lifecycle = std::make_unique<State_Lifecycle>();
	Render = std::make_unique<State_Render>();
	FlyCamera = std::make_unique<State_FlyCamera>();
	Selection = std::make_unique<State_Selection>();
	OverlayMode = std::make_unique<State_OverlayMode>();
	Settings = std::make_unique<State_Settings>();
	Logs = std::make_unique<State_Logs>();
}

void Core_State::InitSources()
{
	MapReader = std::make_unique<State_MapReader>();
	MapBipd = std::make_unique<State_MapBipd<BipdObject>>();
	MapBloc = std::make_unique<State_MapBloc<BlocObject>>();
	MapColl = std::make_unique<State_MapColl<CollObject>>();
	MapCtrl = std::make_unique<State_MapCtrl<CtrlObject>>();
	MapEqip = std::make_unique<State_MapEqip<EqipObject>>();
	MapHlmt = std::make_unique<State_MapHlmt<HlmtObject>>();
	MapLbsp = std::make_unique<State_MapLbsp<LbspObject>>();
	MapMach = std::make_unique<State_MapMach<MachObject>>();
	MapMode = std::make_unique<State_MapMode<ModeObject>>();
	MapPhmo = std::make_unique<State_MapPhmo<PhmoObject>>();
	MapPlay = std::make_unique<State_MapPlay<PlayObject>>();
	MapProj = std::make_unique<State_MapProj<ProjObject>>();
	MapSbsp = std::make_unique<State_MapSbsp<SbspObject>>();
	MapScen = std::make_unique<State_MapScen<ScenObject>>();
	MapScnr = std::make_unique<State_MapScnr<ScnrObject>>();
	MapSldt = std::make_unique<State_MapSldt<SldtObject>>();
	MapVehi = std::make_unique<State_MapVehi<VehiObject>>();
	MapWeap = std::make_unique<State_MapWeap<WeapObject>>();
	MapZone = std::make_unique<State_MapZone<ZoneObject>>();
	ObjectTable = std::make_unique<State_ObjectTable>();
	BoneOffsets = std::make_unique<State_BoneOffsets>();
	BoneMatrices = std::make_unique<State_BoneMatrices>();
	DamageSections = std::make_unique<State_DamageSections>();
	PlayerTable = std::make_unique<State_PlayerTable>();
	InteractionTable = std::make_unique<State_InteractionTable>();
	WorldBuilder = std::make_unique<State_WorldBuilder>();
	StatsBuilder = std::make_unique<State_StatsBuilder>();
	VitalityBuilder = std::make_unique<State_VitalityBuilder>();
}

void Core_State::InitStructure()
{
	ObjectGraph = std::make_unique<State_ObjectGraph>();
	PlayerGraph = std::make_unique<State_PlayerGraph>();
	Classifier = std::make_unique<State_Classifier>();
}

void Core_State::InitEnvironment()
{
	Collidables = std::make_unique<State_Collidables>();
	Vitality = std::make_unique<State_Vitality>();
	Fixtures = std::make_unique<State_Fixtures>();
}

void Core_State::InitEgocentric()
{
	Self = std::make_unique<State_Self>();
	Affordances = std::make_unique<State_Affordances>();
}

void Core_State::Deinitialize()
{
	this->DeinitStructure();
	this->DeinitSources();
	this->DeinitEnvironment();
	this->DeinitEgocentric();

	Tick.reset();

	Input.reset();
	Memory.reset();
	Telemetry.reset();
	Lifecycle.reset();
	Render.reset();
	FlyCamera.reset();
	Settings.reset();
	Logs.reset();
}

void Core_State::DeinitSources()
{
	MapReader.reset();
	MapBipd.reset();
	MapBloc.reset();
	MapColl.reset();
	MapCtrl.reset();
	MapEqip.reset();
	MapHlmt.reset();
	MapLbsp.reset();
	MapMach.reset();
	MapMode.reset();
	MapPhmo.reset();
	MapPlay.reset();
	MapProj.reset();
	MapSbsp.reset();
	MapScen.reset();
	MapScnr.reset();
	MapSldt.reset();
	MapVehi.reset();
	MapWeap.reset();
	MapZone.reset();
	ObjectTable.reset();
	BoneOffsets.reset();
	BoneMatrices.reset();
	DamageSections.reset();
	PlayerTable.reset();
	InteractionTable.reset();
	WorldBuilder.reset();
	StatsBuilder.reset();
	VitalityBuilder.reset();
}

void Core_State::DeinitStructure()
{
	ObjectGraph.reset();
	PlayerGraph.reset();
	Classifier.reset();
}

void Core_State::DeinitEnvironment()
{
	Collidables.reset();
	Vitality.reset();
	Fixtures.reset();
}

void Core_State::DeinitEgocentric()
{
	Self.reset();
	Affordances.reset();
}