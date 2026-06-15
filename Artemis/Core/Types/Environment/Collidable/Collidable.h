#pragma once

#include "Core/Types/Sources/Static/World/CollGeometry.h"
#include "Core/Types/Sources/Static/World/ModeGeometry.h"
#include "Core/Types/Sources/Tables/Object/BoneMatrix/BoneMatrix.h"
#include "Core/Types/Sources/Tables/Object/DamageSection/DamageSection.h"

struct CollidableTriangle
{
    std::array<float, 3> A = {};
    std::array<float, 3> B = {};
    std::array<float, 3> C = {};

    uint8_t SurfaceFlags = 0;
    int16_t Material = -1;
};

struct CollidableMesh
{
    std::vector<CollidableTriangle> Triangles;
};

struct Collidable
{
    uint32_t Handle;
    std::string TagName;

    std::array<float, 3> Position;
    std::array<float, 3> Forward;
    std::array<float, 3> Up;

    const CollGeometry* Coll;
    CollidableMesh CollidableMesh;

    bool AncestorDead = false;
    bool HasDestroyedGeometry = false;

    std::vector<int> RegionToSection;
    std::vector<std::array<int, 5>> StateMap;
    std::vector<std::vector<int>> LevelToState;
    std::vector<int> DeathStateMap;
};