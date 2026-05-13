#pragma once

#include <unordered_map>
#include <cstdint>

struct LiveObject;
enum class ObjectRole : uint8_t;

class System_ObjectClassifier
{
public:
    void UpdateClassification();
    void Cleanup();

private:
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