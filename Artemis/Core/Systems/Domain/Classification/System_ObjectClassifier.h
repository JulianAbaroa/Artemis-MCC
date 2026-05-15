#pragma once

#include <unordered_map>
#include <cstdint>

struct LiveObject;
enum class ObjectRole : uint8_t;

class State_ObjectTable;
class State_PlayerTable;
class State_Classification;
class State_Navigation;
class State_Environment;
class State_Interactable;
class System_Debug;

struct System_ObjectClassifier_Dependencies
{
    State_ObjectTable& State_ObjectTable;
    State_PlayerTable& State_PlayerTable;
    State_Classification& State_Classification;
    State_Navigation& State_Navigation;
    State_Environment& State_Environment;
    State_Interactable& State_Interactable;
    System_Debug& System_Debug;
};

class System_ObjectClassifier
{
public:
    System_ObjectClassifier(System_ObjectClassifier_Dependencies dependencies) :
        m_Deps(dependencies) {}
    ~System_ObjectClassifier() = default;

    void UpdateClassification();

    void Cleanup();

private:
    System_ObjectClassifier_Dependencies m_Deps;

    ObjectRole ClassifyNode(uint32_t handle, 
        uint32_t selfBipedHandle, const LiveObject& node, 
        const std::unordered_map<uint32_t, LiveObject>& nodes) const;

    // --- By ObjectGroup ---

    ObjectRole ClassifyBiped(uint32_t handle, uint32_t selfBipedHandle) const;
    ObjectRole ClassifyVehicle(const LiveObject& object) const;
    ObjectRole ClassifyWeapon(const LiveObject& object) const;
    ObjectRole ClassifyEquipment(const LiveObject& object) const;
    ObjectRole ClassifyScenery(const LiveObject& object) const;
    ObjectRole ClassifyBloc(const LiveObject& object) const;
    ObjectRole ClassifyDeviceControl(const LiveObject& object) const;
};