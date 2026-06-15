#pragma once

#include <unordered_map>
#include <cstdint>
#include <vector>
#include <array>

struct CollMesh;
struct CollObject;
struct HlmtObject;
struct Classified;
struct LiveObject;
struct BoneMatrix;
struct CollGeometry;
struct PhmoRigidBody;
struct CollidableMesh;
struct WorldRigidBody;
struct BoneMatrixTable;
struct DamageSectionTable;
struct Hlmt_VariantsObject;
struct Collidable;

using ObjectTable = std::unordered_map<uint32_t, LiveObject>;
using Classifieds = std::vector<Classified>;
using LevelState = std::vector<std::vector<int>>;

template <typename TObject> class State_MapHlmt; struct HlmtObject;
class State_ObjectTable;
class State_BoneMatrices;
class State_DamageSections;
class State_Classifier;
class State_ObjectGraph;
class State_WorldBuilder;
class State_Collidables;
class System_Logs;

struct Sys_Collidables_Deps
{
    State_MapHlmt<HlmtObject>& State_MapHlmt;
    State_ObjectTable& State_ObjectTable;
    State_BoneMatrices& State_BoneMatrices;
    State_DamageSections& State_DamageSections;
    State_Classifier& State_Classifier;
    State_ObjectGraph& State_ObjectGraph;
    State_WorldBuilder& State_WorldBuilder;
    State_Collidables& State_Collidables;
    System_Logs& System_Logs;
};

class System_Collidables
{
public:
    System_Collidables(Sys_Collidables_Deps deps) : m_Deps(deps) {}
    ~System_Collidables() = default;

    void Update();

    void Cleanup();

private:
    Sys_Collidables_Deps m_Deps;

    void CollectCollidables(const Classifieds& classifieds,
        const ObjectTable& objects);

    std::vector<std::array<int, 5>> CollectStateMap(
        const Hlmt_VariantsObject& variant, const CollGeometry& coll);

    void CollectLevelStateMap(const HlmtObject& hlmt,
        const CollGeometry& coll, LevelState& levelToState);

    void CollectDeathStateMap(const HlmtObject& hlmt,
        const CollGeometry& coll, std::vector<int>& deathState);

    std::vector<int> CollectRegionToSection(const HlmtObject& hlmt,
        const CollGeometry& coll);

    CollidableMesh CollectMesh(const Collidable& instance,
        const BoneMatrixTable* boneMatrixTable, 
        const DamageSectionTable* damageSectionTable);

    CollidableMesh CollectSkeletal(
        const Collidable& instance,
        const BoneMatrixTable& boneMatrixTable,
        const DamageSectionTable* damageSectionTable);

    CollidableMesh CollectRigid(
        const Collidable& instance,
        const DamageSectionTable* damageSectionTable);

    bool IsActivePermutation(const Collidable& instance, 
        const CollMesh& mesh, 
        const DamageSectionTable* damageSectionTable);

    // --- Helpers ---

    std::array<float, 3> Cross(const std::array<float, 3>& a,
        const std::array<float, 3>& b);

    std::array<float, 3> TransformPoint(const std::array<float, 3>& pos,
        const std::array<float, 3>& right, const std::array<float, 3>& forward,
        const std::array<float, 3>& up, float lx, float ly, float lz);

    std::array<float, 3> TransformByBone(const BoneMatrix& m, 
        float lx, float ly, float lz);

    bool IsBoneMatrixValid(const BoneMatrix& m);

    int HighestLevelFromMask(uint16_t mask);

    bool IsAncestorDead(uint32_t handle, const ObjectTable& objects);

    // Debug.
    void LogDamageStructure(const HlmtObject& hlmt, const CollGeometry& coll);
};