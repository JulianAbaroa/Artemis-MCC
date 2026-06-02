#pragma once

#include <unordered_map>
#include <cstdint>
#include <vector>
#include <string>

struct Classified;
struct SbspGeometry;
struct SbspObject;
struct BlocResult;
struct LiveObject;
struct AINavigationCluster;
struct ScnrTriggerVolume;

class State_MapReader;
class State_MapSbsp;
class State_MapScen;
class State_MapBloc;
class State_MapMach;
class State_MapScnr;
class State_ObjectTable;
class State_Classifier;
class State_Navigation;
class System_SbspBuilder;
class System_SbspSeamLinker;
class System_ScnrBuilder;
class System_Logs;

class System_MeshReader;

struct System_Navigation_Dependencies
{
    State_MapReader& State_Map;
    State_MapSbsp& State_MapSbsp;
    State_MapScen& State_MapScen;
    State_MapBloc& State_MapBloc;
    State_MapMach& State_MapMach;
    State_MapScnr& State_MapScnr;
    State_ObjectTable& State_ObjectTable;
    State_Classifier& State_Classification;
    State_Navigation& State_Navigation;
    System_SbspBuilder& System_SbspBuilder;
    System_SbspSeamLinker& System_SbspSeamLinker;
    System_ScnrBuilder& System_ScnrBuilder;
    System_MeshReader& System_MeshReader;
    System_Logs& System_Logs;
};

class System_Navigation
{
public:
    System_Navigation(System_Navigation_Dependencies dependencies) :
        m_Deps(dependencies) {}
    ~System_Navigation() = default;

    // --- Static Data ---

    void BuildForMap();

    // --- Dynamic Data ---

    void UpdateNavigation();

    void Cleanup();

private:
    System_Navigation_Dependencies m_Deps;

    // --- Static Data ---

    bool BuildSbsp(const std::string& tagName,
        std::vector<SbspGeometry>& geometries, 
        std::vector<const SbspObject*>& sbspObjects);

    bool BuildScnr(const std::string& tagName);

    void BuildNavigationGraph(
        const std::vector<SbspGeometry>& geometries) const;

    bool ClusterOverlapsVolume(const AINavigationCluster& cluster,
        const ScnrTriggerVolume& volume) const;

    // --- Dynamic Data ---

    void BuildObstacles(const std::vector<Classified>& classifieds,
        const std::unordered_map<uint32_t, LiveObject> objects);

    void BuildSpawns(const std::vector<Classified>& classifieds,
        const std::unordered_map<uint32_t, LiveObject> objects);

    void BuildTeleports(const std::vector<Classified>& classifieds,
        const std::unordered_map<uint32_t, LiveObject> objects);

    void BuildLifts(const std::vector<Classified>& classifieds,
        const std::unordered_map<uint32_t, LiveObject> objects);

    void BuildShields(const std::vector<Classified>& classifieds,
        const std::unordered_map<uint32_t, LiveObject> objects);

    void BuildObjectives(const std::vector<Classified>& classifieds,
        const std::unordered_map<uint32_t, LiveObject> objects);

    void BuildDestructibles(const std::vector<Classified>& classifieds,
        const std::unordered_map<uint32_t, LiveObject> objects);
};