#pragma once

#include <unordered_map>
#include <cstdint>

struct LiveObject;
enum class ObjectRole : uint8_t;

using ObjectTable = std::unordered_map<uint32_t, LiveObject>;

class State_ObjectTable;
class State_PlayerTable;
class State_Classifier;
class System_Logs;

struct Sys_Classifier_Deps
{
    State_ObjectTable& State_ObjectTable;
    State_PlayerTable& State_PlayerTable;
    State_Classifier& State_Classifier;
    System_Logs& System_Logs;
};

class System_Classifier
{
public:
    System_Classifier(Sys_Classifier_Deps deps) : m_Deps(deps) {}
    ~System_Classifier() = default;

    void UpdateClassification();

    void Cleanup();

private:
    Sys_Classifier_Deps m_Deps;

    ObjectRole ClassifyNode(uint32_t handle, const LiveObject& node, 
        const std::unordered_map<uint32_t, LiveObject>& nodes) const;

    ObjectRole ClassifyVehicle(const LiveObject& object) const;
    ObjectRole ClassifyWeapon(const LiveObject& object) const;
    ObjectRole ClassifyEquipment(const LiveObject& object) const;
    ObjectRole ClassifyScenery(const LiveObject& object) const;
    ObjectRole ClassifyBloc(const LiveObject& object) const;
    ObjectRole ClassifyDeviceControl(const LiveObject& object) const;
};