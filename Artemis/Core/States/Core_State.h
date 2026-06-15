#pragma once

#include <memory>

// --- Layer 0: Sources ---
class State_MapReader; 
template <typename TObject> class State_MapBipd; struct BipdObject;
template <typename TObject> class State_MapBloc; struct BlocObject;
template <typename TObject> class State_MapColl; struct CollObject;
template <typename TObject> class State_MapCtrl; struct CtrlObject;
template <typename TObject> class State_MapEqip; struct EqipObject;
template <typename TObject> class State_MapLbsp; struct LbspObject;
template <typename TObject> class State_MapMach; struct MachObject;
template <typename TObject> class State_MapMode; struct ModeObject;
template <typename TObject> class State_MapHlmt; struct HlmtObject;
template <typename TObject> class State_MapPhmo; struct PhmoObject;
template <typename TObject> class State_MapPlay; struct PlayObject;
template <typename TObject> class State_MapProj; struct ProjObject;
template <typename TObject> class State_MapSbsp; struct SbspObject;
template <typename TObject> class State_MapScen; struct ScenObject;
template <typename TObject> class State_MapScnr; struct ScnrObject;
template <typename TObject> class State_MapSldt; struct SldtObject;
template <typename TObject> class State_MapVehi; struct VehiObject; 
template <typename TObject> class State_MapWeap; struct WeapObject;
template <typename TObject> class State_MapZone; struct ZoneObject;
class State_ObjectTable; class State_BoneOffsets;
class State_BoneMatrices; class State_DamageSections;
class State_PlayerTable; class State_InteractionTable;
class State_WorldBuilder; class State_StatsBuilder;
class State_VitalityBuilder;

// --- Layer 1: Structure ---
class State_ObjectGraph; class State_PlayerGraph;
class State_Classifier;

// --- Layer 2: Environment ---
class State_Collidables; class State_Vitality;
class State_Fixtures;

// --- Layer 3: Egocentric ---
class State_Self; class State_Affordances;

// Tick.
class State_Tick;

// Other.
class State_Input; class State_MemoryScanner;
class State_Telemetry; class State_Lifecycle; 
class State_Render; class State_FlyCamera;
class State_Selection; class State_OverlayMode;
class State_Settings; class State_Logs;

class Core_State
{
public:
	Core_State();
	~Core_State();

	void Initialize();	
	void Deinitialize();

	// --- Layer 0: Sources ---
	std::unique_ptr<State_MapReader> MapReader;
	std::unique_ptr<State_MapBipd<BipdObject>> MapBipd;
	std::unique_ptr<State_MapBloc<BlocObject>> MapBloc;
	std::unique_ptr<State_MapColl<CollObject>> MapColl;
	std::unique_ptr<State_MapCtrl<CtrlObject>> MapCtrl;
	std::unique_ptr<State_MapEqip<EqipObject>> MapEqip;
	std::unique_ptr<State_MapHlmt<HlmtObject>> MapHlmt;
	std::unique_ptr<State_MapLbsp<LbspObject>> MapLbsp;
	std::unique_ptr<State_MapMach<MachObject>> MapMach;
	std::unique_ptr<State_MapMode<ModeObject>> MapMode;
	std::unique_ptr<State_MapPhmo<PhmoObject>> MapPhmo;
	std::unique_ptr<State_MapPlay<PlayObject>> MapPlay;
	std::unique_ptr<State_MapProj<ProjObject>> MapProj;
	std::unique_ptr<State_MapSbsp<SbspObject>> MapSbsp;
	std::unique_ptr<State_MapScen<ScenObject>> MapScen;
	std::unique_ptr<State_MapScnr<ScnrObject>> MapScnr;
	std::unique_ptr<State_MapSldt<SldtObject>> MapSldt;
	std::unique_ptr<State_MapVehi<VehiObject>> MapVehi;
	std::unique_ptr<State_MapWeap<WeapObject>> MapWeap;
	std::unique_ptr<State_MapZone<ZoneObject>> MapZone;
	std::unique_ptr<State_ObjectTable> ObjectTable;
	std::unique_ptr<State_BoneOffsets> BoneOffsets;
	std::unique_ptr<State_BoneMatrices> BoneMatrices;
	std::unique_ptr<State_DamageSections> DamageSections;
	std::unique_ptr<State_PlayerTable> PlayerTable;
	std::unique_ptr<State_InteractionTable> InteractionTable;
	std::unique_ptr<State_WorldBuilder> WorldBuilder;
	std::unique_ptr<State_StatsBuilder> StatsBuilder;
	std::unique_ptr<State_VitalityBuilder> VitalityBuilder;

	// --- Layer 1: Structure ---
	std::unique_ptr<State_ObjectGraph> ObjectGraph;
	std::unique_ptr<State_PlayerGraph> PlayerGraph;
	std::unique_ptr<State_Classifier> Classifier;

	// --- Layer 2: Environment ---
	std::unique_ptr<State_Collidables> Collidables;
	std::unique_ptr<State_Vitality> Vitality;
	std::unique_ptr<State_Fixtures> Fixtures;

	// --- Layer 3: Egocentric ---
	std::unique_ptr<State_Self> Self;
	std::unique_ptr<State_Affordances> Affordances;

	// Tick.
	std::unique_ptr<State_Tick> Tick;

	// Other
	std::unique_ptr<State_Input> Input;
	std::unique_ptr<State_MemoryScanner> Memory;
	std::unique_ptr<State_Telemetry> Telemetry;
	std::unique_ptr<State_Lifecycle> Lifecycle;
	std::unique_ptr<State_Render> Render;
	std::unique_ptr<State_FlyCamera> FlyCamera;
	std::unique_ptr<State_Selection> Selection;
	std::unique_ptr<State_OverlayMode> OverlayMode;
	std::unique_ptr<State_Settings> Settings;
	std::unique_ptr<State_Logs> Logs;

private:
	void InitSources();
	void DeinitSources();

	void InitStructure();
	void DeinitStructure();

	void InitEnvironment();
	void DeinitEnvironment();

	void InitEgocentric();
	void DeinitEgocentric();
};