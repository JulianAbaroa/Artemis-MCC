#pragma once

// Types.
#include "Core/Types/Domain/Environment/CollGeometry.h"
#include "Core/Types/Domain/Environment/PhmoGeometry.h"
#include "Core/Types/Domain/Environment/ModeGeometry.h"

#include <cstdint>
#include <string>
#include <array>

// ----- Dynamic Data -----

enum class WorldShapeType : uint8_t
{
    Sphere = 0,
    Pill = 1,
    Box = 2,
    Polyhedron = 3,
    MultiSphere = 4,
};

struct WorldShape
{
    WorldShapeType Type;

    // Sphere / MultiSphere individual sphere
    std::array<float, 3> SphereCenter; // world-space
    float                SphereRadius;

    // Pill
    std::array<float, 3> PillBottom;   // world-space
    std::array<float, 3> PillTop;      // world-space
    float                PillRadius;

    // Box — 8 corners world-space
    std::array<std::array<float, 3>, 8> BoxCorners;

    // Polyhedron
    std::vector<std::array<float, 3>> PolyhedronVertices; // world-space
};

struct WorldRigidBody
{
    std::array<float, 3> BoundingSphereCenter; // world-space
    float                BoundingSphereRadius;
    std::vector<WorldShape> Shapes;
};

// Represents a world object with its physical geometry resolved,
// posed at its current world-space transform.
// TODO: Add CollGeometry* and ModeGeometry* for full shape coverage.
struct ActivePhysicsInstance
{
    uint32_t Handle;
    std::string TagName;

    std::array<float, 3> Position;
    std::array<float, 3> Forward;
    std::array<float, 3> Up;

    // AABB in local space, from CollGeometry. 
    // Use with pose for broad-phase.
    std::array<float, 3> CollBoundsMin;
    std::array<float, 3> CollBoundsMax;

    // Resolved rigid bodies in local space. 
    // Use with pose for narrow-phase.
    // Null if no PhmoGeometry was found for this tag.
    const PhmoGeometry* Phmo = nullptr;

    // World-space rigid bodies, ready for AI and UI consumption.
    std::vector<WorldRigidBody> WorldRigidBodies;
};