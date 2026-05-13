#pragma once

#include <unordered_map>
#include <cstdint>
#include <vector>
#include <string>

class System_Debug;
class State_Navigation;

struct ClassifiedObject;
struct SbspGeometry;
struct SbspObject;
struct BlocResult;
struct LiveObject;

class System_Navigation
{
public:
    // --- Static Data ---

    void BuildForMap();

    // --- Dynamic Data ---

    void UpdateNavigation();

    // Cleanup.
    void Cleanup();

private:
    const float m_MinVolume = 0.001f;
    const float m_MaxVolume = 200.0f * 200.0f * 200.0f;

    // --- Static Data ---

    bool BuildSbsp(const std::string& tagName, System_Debug& debug,
        std::vector<SbspGeometry>& geometries, 
        std::vector<const SbspObject*>& sbspObjects);

    bool BuildScen(const std::string& tagName, System_Debug& debug,
        State_Navigation& navigation);

    BlocResult BuildBloc(const std::string& tagName, System_Debug& debug,
        State_Navigation& navigation);

    bool BuildMach(const std::string& tagName, System_Debug& debug,
        State_Navigation& navigation);

    void BuildNavigationGraph(const std::vector<SbspGeometry>& geometries,
        State_Navigation& navigation, System_Debug& debug) const;

    // --- Dynamic Data ---

    void BuildObstacles(State_Navigation& navigation,
        const std::vector<ClassifiedObject>& classifieds,
        const std::unordered_map<uint32_t, LiveObject> objects);

    void BuildSpawns(State_Navigation& navigation,
        const std::vector<ClassifiedObject>& classifieds,
        const std::unordered_map<uint32_t, LiveObject> objects);

    void BuildTeleports(State_Navigation& navigation,
        const std::vector<ClassifiedObject>& classifieds,
        const std::unordered_map<uint32_t, LiveObject> objects);

    void BuildLifts(State_Navigation& navigation,
        const std::vector<ClassifiedObject>& classifieds,
        const std::unordered_map<uint32_t, LiveObject> objects);

    void BuildShields(State_Navigation& navigation,
        const std::vector<ClassifiedObject>& classifieds,
        const std::unordered_map<uint32_t, LiveObject> objects);

    void BuildObjectives(State_Navigation& navigation,
        const std::vector<ClassifiedObject>& classifieds,
        const std::unordered_map<uint32_t, LiveObject> objects);

    void BuildDestructibles(State_Navigation& navigation,
        const std::vector<ClassifiedObject>& classifieds,
        const std::unordered_map<uint32_t, LiveObject> objects);
};