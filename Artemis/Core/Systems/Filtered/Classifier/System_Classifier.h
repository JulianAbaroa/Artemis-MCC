#pragma once

#include <unordered_map>
#include <cstdint>

struct LiveObject;
enum class ObjectRole : uint8_t;

class State_ObjectTable;
class State_PlayerTable;
class State_Classifier;
class State_Navigation;
class State_Environment;
class State_Interactable;
class System_Logs;

struct System_Classifier_Dependencies
{
    State_ObjectTable& State_ObjectTable;
    State_PlayerTable& State_PlayerTable;
    State_Classifier& State_Classification;
    State_Navigation& State_Navigation;
    State_Environment& State_Environment;
    State_Interactable& State_Interactable;
    System_Logs& System_Logs;
};

class System_Classifier
{
public:
    System_Classifier(System_Classifier_Dependencies dependencies) :
        m_Deps(dependencies) {}
    ~System_Classifier() = default;

    void UpdateClassification();

    void Cleanup();

private:
    System_Classifier_Dependencies m_Deps;

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