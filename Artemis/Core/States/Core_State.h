#pragma once

#include <memory>

// Related to .map files.
class State_MapReader; class State_MapBipd; class State_MapBloc; 
class State_MapColl; class State_MapCtrl; class State_MapEqip; 
class State_MapJmad; class State_MapLbsp; class State_MapMach; 
class State_MapMode; class State_MapPhmo; class State_MapPlay; 
class State_MapProj; class State_MapSbsp; class State_MapScen; 
class State_MapScnr; class State_MapSldt; class State_MapVehi; 
class State_MapWeap; class State_MapZone;

// Related to game engine tables.
class State_ObjectTable; class State_PlayerTable; 
class State_InteractionTable;

// Related to data specialization.
class State_ObjectGraph; class State_PlayerGraph;
class State_Classifier;

// Related to AI domains.
class State_Navigation; class State_Environment; 
class State_Interactable;

// Other.
class State_Input; class State_MemoryScanner;
class State_Lifecycle; class State_Render;
class State_Settings; class State_Logs;

class Core_State
{
public:
	Core_State();
	~Core_State();

	void Initialize();	
	void Deinitialize();

	std::unique_ptr<State_MapReader> MapReader;
	std::unique_ptr<State_MapBipd> MapBipd;
	std::unique_ptr<State_MapBloc> MapBloc;
	std::unique_ptr<State_MapColl> MapColl;
	std::unique_ptr<State_MapCtrl> MapCtrl;
	std::unique_ptr<State_MapEqip> MapEqip;
	std::unique_ptr<State_MapJmad> MapJmad;
	std::unique_ptr<State_MapLbsp> MapLbsp;
	std::unique_ptr<State_MapMach> MapMach;
	std::unique_ptr<State_MapMode> MapMode;
	std::unique_ptr<State_MapPhmo> MapPhmo;
	std::unique_ptr<State_MapPlay> MapPlay;
	std::unique_ptr<State_MapProj> MapProj;
	std::unique_ptr<State_MapSbsp> MapSbsp;
	std::unique_ptr<State_MapScen> MapScen;
	std::unique_ptr<State_MapScnr> MapScnr;
	std::unique_ptr<State_MapSldt> MapSldt;
	std::unique_ptr<State_MapVehi> MapVehi;
	std::unique_ptr<State_MapWeap> MapWeap;
	std::unique_ptr<State_MapZone> MapZone;

	std::unique_ptr<State_ObjectTable> ObjectTable;
	std::unique_ptr<State_PlayerTable> PlayerTable;
	std::unique_ptr<State_InteractionTable> InteractionTable;

	std::unique_ptr<State_ObjectGraph> ObjectGraph;
	std::unique_ptr<State_PlayerGraph> PlayerGraph;
	std::unique_ptr<State_Classifier> Classifier;

	std::unique_ptr<State_Navigation> Navigation;
	std::unique_ptr<State_Environment> Environment;
	std::unique_ptr<State_Interactable> Interactable;

	std::unique_ptr<State_Input> Input;
	std::unique_ptr<State_MemoryScanner> Memory;
	std::unique_ptr<State_Lifecycle> Lifecycle;
	std::unique_ptr<State_Render> Render;
	std::unique_ptr<State_Settings> Settings;
	std::unique_ptr<State_Logs> Logs;

private:
	void InitMapReaders();
	void DeinitMapReaders();

	void InitTables();
	void DeinitTables();

	void InitFiltered();
	void DeinitFiltered();

	void InitDomains();
	void DeinitDomains();
};