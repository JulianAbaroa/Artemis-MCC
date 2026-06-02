#include "pch.h"

#include "Core_State.h"

#include "MapReader/State_MapReader.h"
#include "MapReader/Bipd/State_MapBipd.h" 
#include "MapReader/Bloc/State_MapBloc.h"
#include "MapReader/Coll/State_MapColl.h"
#include "MapReader/Ctrl/State_MapCtrl.h"
#include "MapReader/Eqip/State_MapEqip.h"
#include "MapReader/Jmad/State_MapJmad.h"
#include "MapReader/Lbsp/State_MapLbsp.h"
#include "MapReader/Mach/State_MapMach.h"
#include "MapReader/Mode/State_MapMode.h"
#include "MapReader/Phmo/State_MapPhmo.h"
#include "MapReader/Play/State_MapPlay.h"
#include "MapReader/Proj/State_MapProj.h"
#include "MapReader/Sbsp/State_MapSbsp.h"
#include "MapReader/Sbsp/State_MapSbsp.h"
#include "MapReader/Scen/State_MapScen.h"
#include "MapReader/Scnr/State_MapScnr.h"
#include "MapReader/Sldt/State_MapSldt.h"
#include "MapReader/Vehi/State_MapVehi.h"
#include "MapReader/Weap/State_MapWeap.h"
#include "MapReader/Zone/State_MapZone.h"

#include "Tables/Object/State_ObjectTable.h"
#include "Tables/Player/State_PlayerTable.h"
#include "Tables/Interaction/State_InteractionTable.h"

#include "Filtered/Graph/Object/State_ObjectGraph.h"
#include "Filtered/Graph/Player/State_PlayerGraph.h"
#include "Filtered/Classifier/State_Classifier.h"

#include "Domains/Navigation/State_Navigation.h"
#include "Domains/Environment/State_Environment.h"
#include "Domains/Interactable/State_Interactable.h"

#include "Input/State_Input.h"
#include "Memory/State_MemoryScanner.h"
#include "Lifecycle/State_Lifecycle.h"
#include "Render/State_Render.h"
#include "Settings/State_Settings.h"
#include "Logs/State_Logs.h"

Core_State::Core_State() = default;
Core_State::~Core_State() = default;

void Core_State::Initialize()
{
	this->InitMapReaders();
	this->InitTables();
	this->InitFiltered();
	this->InitDomains();

	Input = std::make_unique<State_Input>();
	Memory = std::make_unique<State_MemoryScanner>();
	Lifecycle = std::make_unique<State_Lifecycle>();
	Render = std::make_unique<State_Render>();
	Settings = std::make_unique<State_Settings>();
	Logs = std::make_unique<State_Logs>();
}

void Core_State::InitMapReaders()
{
	MapReader = std::make_unique<State_MapReader>();
	MapBipd = std::make_unique<State_MapBipd>();
	MapBloc = std::make_unique<State_MapBloc>();
	MapColl = std::make_unique<State_MapColl>();
	MapCtrl = std::make_unique<State_MapCtrl>();
	MapEqip = std::make_unique<State_MapEqip>();
	MapJmad = std::make_unique<State_MapJmad>();
	MapLbsp = std::make_unique<State_MapLbsp>();
	MapMach = std::make_unique<State_MapMach>();
	MapMode = std::make_unique<State_MapMode>();
	MapPhmo = std::make_unique<State_MapPhmo>();
	MapPlay = std::make_unique<State_MapPlay>();
	MapProj = std::make_unique<State_MapProj>();
	MapSbsp = std::make_unique<State_MapSbsp>();
	MapScen = std::make_unique<State_MapScen>();
	MapScnr = std::make_unique<State_MapScnr>();
	MapSldt = std::make_unique<State_MapSldt>();
	MapVehi = std::make_unique<State_MapVehi>();
	MapWeap = std::make_unique<State_MapWeap>();
	MapZone = std::make_unique<State_MapZone>();
}

void Core_State::InitTables()
{
	ObjectTable = std::make_unique<State_ObjectTable>();
	PlayerTable = std::make_unique<State_PlayerTable>();
	InteractionTable = std::make_unique<State_InteractionTable>();
}

void Core_State::InitFiltered()
{
	ObjectGraph = std::make_unique<State_ObjectGraph>(*ObjectTable);
	PlayerGraph = std::make_unique<State_PlayerGraph>();
	Classifier = std::make_unique<State_Classifier>();
}

void Core_State::InitDomains()
{
	Navigation = std::make_unique<State_Navigation>();
	Environment = std::make_unique<State_Environment>();
	Interactable = std::make_unique<State_Interactable>();
}

void Core_State::Deinitialize()
{
	this->DeinitMapReaders();
	this->DeinitTables();
	this->DeinitFiltered();
	this->DeinitDomains();

	Input.reset();
	Memory.reset();
	Lifecycle.reset();
	Render.reset();
	Settings.reset();
	Logs.reset();
}

void Core_State::DeinitMapReaders()
{
	MapReader.reset();
	MapBipd.reset();
	MapBloc.reset();
	MapColl.reset();
	MapCtrl.reset();
	MapEqip.reset();
	MapJmad.reset();
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
}

void Core_State::DeinitTables()
{
	ObjectTable.reset();
	PlayerTable.reset();
	InteractionTable.reset();
}

void Core_State::DeinitFiltered()
{
	ObjectGraph.reset();
	PlayerGraph.reset();
	Classifier.reset();
}

void Core_State::DeinitDomains()
{
	Navigation.reset();
	Environment.reset();
	Interactable.reset();
}