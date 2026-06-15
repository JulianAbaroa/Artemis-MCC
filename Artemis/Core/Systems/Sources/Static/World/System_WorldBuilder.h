#pragma once

#include <string>
#include <vector>

struct SbspObject;
struct SbspGeometry;

using SbspGeometries = std::vector<SbspGeometry>;
using SbspObjects = std::vector<const SbspObject*>;

class State_MapReader;
template <typename TObject> class MapTag;
template <typename TObject> class State_MapBipd; struct BipdObject;
template <typename TObject> class State_MapBloc; struct BlocObject;
template <typename TObject> class State_MapColl; struct CollObject;
template <typename TObject> class State_MapCtrl; struct CtrlObject;
template <typename TObject> class State_MapEqip; struct EqipObject;
template <typename TObject> class State_MapHlmt; struct HlmtObject;
template <typename TObject> class State_MapMach; struct MachObject;
template <typename TObject> class State_MapMode; struct ModeObject;
template <typename TObject> class State_MapPhmo; struct PhmoObject;
template <typename TObject> class State_MapSbsp; struct SbspObject;
template <typename TObject> class State_MapScen; struct ScenObject;
template <typename TObject> class State_MapVehi; struct VehiObject;
template <typename TObject> class State_MapWeap; struct WeapObject;
class State_WorldBuilder;
class System_MapReader;
class System_SbspBuilder;
class System_SbspSeamLinker;
class System_CollBuilder;
class System_PhmoBuilder;
class System_ModeBuilder;
class System_GeometryReader;
class System_Logs;

struct Sys_WorldBuilder_Deps
{
    State_MapReader& State_MapReader;
    State_MapBipd<BipdObject>& State_MapBipd;
    State_MapBloc<BlocObject>& State_MapBloc;
    State_MapColl<CollObject>& State_MapColl;
    State_MapCtrl<CtrlObject>& State_MapCtrl;
    State_MapEqip<EqipObject>& State_MapEqip;
    State_MapHlmt<HlmtObject>& State_MapHlmt;
    State_MapMach<MachObject>& State_MapMach;
    State_MapMode<ModeObject>& State_MapMode;
    State_MapPhmo<PhmoObject>& State_MapPhmo;
    State_MapSbsp<SbspObject>& State_MapSbsp;
    State_MapScen<ScenObject>& State_MapScen;
    State_MapVehi<VehiObject>& State_MapVehi;
    State_MapWeap<WeapObject>& State_MapWeap;
    State_WorldBuilder& State_WorldBuilder;
    System_MapReader& System_MapReader;
    System_SbspSeamLinker& System_SbspSeamLinker;
    System_SbspBuilder& System_SbspBuilder;
    System_CollBuilder& System_CollBuilder;
    System_PhmoBuilder& System_PhmoBuilder;
    System_ModeBuilder& System_ModeBuilder;
    System_GeometryReader& System_GeometryReader;
    System_Logs& System_Logs;
};

class System_WorldBuilder
{
public:
    System_WorldBuilder(Sys_WorldBuilder_Deps deps) : m_Deps(deps) {}
    ~System_WorldBuilder() = default;

    void BuildForMap();

    void Cleanup();

private:
    Sys_WorldBuilder_Deps m_Deps;

    bool BuildColl(const std::string& tagName);
    bool BuildMode(const std::string& tagName);

    bool BuildSbsp(const std::string& tagName,
        SbspGeometries& geometries, SbspObjects& sbspObjects);

    void BuildNavigationGraph(const SbspGeometries& geometries) const;

    void LinkObjectColls();
    template <typename TObject>
    void LinkObjectFamily(const MapTag<TObject>& state);
};