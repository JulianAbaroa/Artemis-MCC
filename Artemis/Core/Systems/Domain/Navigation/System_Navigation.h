#pragma once

#include <unordered_map>
#include <cstdint>
#include <vector>
#include <string>

struct ClassifiedObject;
struct SbspGeometry;
struct SbspObject;
struct BlocResult;
struct LiveObject;

class State_Map;
class State_MapSbsp;
class State_MapScen;
class State_MapBloc;
class State_MapMach;
class State_ObjectTable;
class State_Classification;
class State_Navigation;
class System_SbspGeometryBuilder;
class System_SbspSeamLinker;
class System_ScenObstacleBuilder;
class System_BlocObstacleBuilder;
class System_BlocTeleporterBuilder;
class System_MachDataBuilder;
class System_Debug;

struct System_Navigation_Dependencies
{
    State_Map& State_Map;
    State_MapSbsp& State_MapSbsp;
    State_MapScen& State_MapScen;
    State_MapBloc& State_MapBloc;
    State_MapMach& State_MapMach;
    State_ObjectTable& State_ObjectTable;
    State_Classification& State_Classification;
    State_Navigation& State_Navigation;
    System_SbspGeometryBuilder& System_SbspGeometryBuilder;
    System_SbspSeamLinker& System_SbspSeamLinker;
    System_ScenObstacleBuilder& System_ScenObstacleBuilder;
    System_BlocObstacleBuilder& System_BlocObstacleBuilder;
    System_BlocTeleporterBuilder& System_BlocTeleporterBuilder;
    System_MachDataBuilder& System_MachDataBuilder;
    System_Debug& System_Debug;
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

    // Cleanup.
    void Cleanup();

private:
    System_Navigation_Dependencies m_Deps;

    const float m_MinVolume = 0.001f;
    const float m_MaxVolume = 200.0f * 200.0f * 200.0f;

    // --- Static Data ---

    bool BuildSbsp(const std::string& tagName,
        std::vector<SbspGeometry>& geometries, 
        std::vector<const SbspObject*>& sbspObjects);

    bool BuildScen(const std::string& tagName);
    BlocResult BuildBloc(const std::string& tagName);
    bool BuildMach(const std::string& tagName);

    void BuildNavigationGraph(
        const std::vector<SbspGeometry>& geometries) const;

    // --- Dynamic Data ---

    void BuildObstacles(const std::vector<ClassifiedObject>& classifieds,
        const std::unordered_map<uint32_t, LiveObject> objects);

    void BuildSpawns(const std::vector<ClassifiedObject>& classifieds,
        const std::unordered_map<uint32_t, LiveObject> objects);

    void BuildTeleports(const std::vector<ClassifiedObject>& classifieds,
        const std::unordered_map<uint32_t, LiveObject> objects);

    void BuildLifts(const std::vector<ClassifiedObject>& classifieds,
        const std::unordered_map<uint32_t, LiveObject> objects);

    void BuildShields(const std::vector<ClassifiedObject>& classifieds,
        const std::unordered_map<uint32_t, LiveObject> objects);

    void BuildObjectives(const std::vector<ClassifiedObject>& classifieds,
        const std::unordered_map<uint32_t, LiveObject> objects);

    void BuildDestructibles(const std::vector<ClassifiedObject>& classifieds,
        const std::unordered_map<uint32_t, LiveObject> objects);
};