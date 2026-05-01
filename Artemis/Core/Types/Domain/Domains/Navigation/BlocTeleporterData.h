#pragma once

#include <string>
#include <cstdint>

enum class BlocTeleporterRole : uint8_t
{
    TwoWay = 12,
    Sender = 13,
    Receiver = 14,
};

enum class BlocBoundaryShape : uint8_t
{
    Unused = 0,
    Sphere = 1,
    Cylinder = 2,
    Box = 3,
};

struct BlocTeleporterData
{
    std::string TagName;

    BlocTeleporterRole Role;
    BlocBoundaryShape BoundaryShape;

    // Cylinder: WidthRadius = radius, BoxLength unused
    // Box:      WidthRadius = half-width X, BoxLength = half-width Y
    float BoundaryWidthRadius;
    float BoundaryBoxLength;
    float BoundaryHeight;

    int16_t DefaultSpawnTime;
    int16_t DefaultAbandonmentTime;
};