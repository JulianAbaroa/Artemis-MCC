#pragma once

#include <cstdint>

struct LiveObject;
struct ObjectProfile;

template <typename TObject> class State_MapBipd; struct BipdObject;
template <typename TObject> class State_MapBloc; struct BlocObject;
template <typename TObject> class State_MapColl; struct CollObject;
template <typename TObject> class State_MapCtrl; struct CtrlObject;
template <typename TObject> class State_MapEqip; struct EqipObject;
template <typename TObject> class State_MapHlmt; struct HlmtObject;
template <typename TObject> class State_MapLbsp; struct LbspObject;
template <typename TObject> class State_MapMach; struct MachObject;
template <typename TObject> class State_MapMode; struct ModeObject;
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
class State_ObjectTable;
class State_BoneOffsets;
class State_BoneMatrices;
class State_DamageSections;
class State_WorldBuilder;
class System_MapReader;
class System_MemoryReader;
class System_Logs;

struct Sys_ObjectTable_Deps
{
	State_MapBipd<BipdObject>& State_MapBipd;
	State_MapBloc<BlocObject>& State_MapBloc;
	State_MapColl<CollObject>& State_MapColl;
	State_MapCtrl<CtrlObject>& State_MapCtrl;
	State_MapEqip<EqipObject>& State_MapEqip;
	State_MapHlmt<HlmtObject>& State_MapHlmt;
	State_MapLbsp<LbspObject>& State_MapLbsp;
	State_MapMach<MachObject>& State_MapMach;
	State_MapMode<ModeObject>& State_MapMode;
	State_MapPhmo<PhmoObject>& State_MapPhmo;
	State_MapPlay<PlayObject>& State_MapPlay;
	State_MapProj<ProjObject>& State_MapProj;
	State_MapSbsp<SbspObject>& State_MapSbsp;
	State_MapScen<ScenObject>& State_MapScen;
	State_MapScnr<ScnrObject>& State_MapScnr;
	State_MapSldt<SldtObject>& State_MapSldt;
	State_MapVehi<VehiObject>& State_MapVehi;
	State_MapWeap<WeapObject>& State_MapWeap;
	State_MapZone<ZoneObject>& State_MapZone;
	State_ObjectTable& State_ObjectTable;
	State_BoneOffsets& State_BoneOffsets;
	State_BoneMatrices& State_BoneMatrices;
	State_DamageSections& State_DamageSections;
	State_WorldBuilder& State_WorldBuilder;
	System_MapReader& System_MapReader;
	System_MemoryReader& System_MemoryReader;
	System_Logs& System_Logs;
};

class System_ObjectTable
{
public:
	System_ObjectTable(Sys_ObjectTable_Deps deps) : m_Deps(deps) {}
	~System_ObjectTable() = default;

	void OnObjectCreated(uint32_t handle, uint32_t datumIndex);
	void OnObjectDestroyed(uint32_t handle);

	void UpdateObjectTable();

	void Cleanup();

private:
	Sys_ObjectTable_Deps m_Deps;

	static constexpr uint8_t m_kDamageSectionStride = 0x18;

	void UpdateObjectData(LiveObject& object);

	void ReadBoneMatrixTable(System_MemoryReader& reader, LiveObject& object);
	void ReadDamageSectionTable(System_MemoryReader& reader, LiveObject& object);

	void UpdateBiped(System_MemoryReader& reader, LiveObject& object);
	void UpdateVehicle(System_MemoryReader& reader, LiveObject& object);
	void UpdateWeapon(System_MemoryReader& reader, LiveObject& object);
	void UpdateEquipment(System_MemoryReader& reader, LiveObject& object);
	void UpdateProjectiles(System_MemoryReader& reader, LiveObject& object);
	void UpdateCrate(System_MemoryReader& reader, LiveObject& object);
	void UpdateScenery(System_MemoryReader& reader, LiveObject& object);

	void SetProfile(LiveObject& object, ObjectProfile& profile);
};