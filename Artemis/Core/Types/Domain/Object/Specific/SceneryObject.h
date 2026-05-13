#pragma once

#include "../../Team.h"
#include "../../TagName.h"
#include "../../ZoneShape.h"
#include <unordered_map>
#include <optional>
#include <cstdint>
#include <string>

enum class SceneryType : uint8_t
{
	Unknown = 0,

	// Spawn.
    InvisibleRespawnPoint,
    InitialSpawnPoint,
    RespawnPoint,

    // Boundaries.
    SafeBoundary,
    SoftSafeBoundary,
    KillBoundary,
    SoftKillBoundary,

    // Portable Shield.
    PortableShield,
};

struct SpawnData
{
    Team Team;
};

struct BoundaryData
{
    ZoneShape Shape;
    Team Team;
};

struct PortableShieldData
{
    uint16_t RegenerationDelay;
    float Health;
};

namespace SceneryObjectHelpers
{
    inline SceneryType ResolveSceneryType(const std::string& tagName)
    {
        using namespace TagName;

        static const std::unordered_map<std::string, SceneryType> map =
        {
            { Spawn::InvisibleRespawnPoint, SceneryType::InvisibleRespawnPoint  },
            { Spawn::InitialSpawnPoint,     SceneryType::InitialSpawnPoint      },
            { Spawn::RespawnPoint,          SceneryType::RespawnPoint           },
            { Boundary::SafeBoundary,       SceneryType::SafeBoundary           },
            { Boundary::SoftSafeBoundary,   SceneryType::SoftSafeBoundary       },
            { Boundary::KillBoundary,       SceneryType::KillBoundary           },
            { Boundary::SoftKillBoundary,   SceneryType::SoftKillBoundary       },
            { Shield::PortableShield,       SceneryType::PortableShield         },
        };

        auto it = map.find(tagName);
        return it != map.end() ? it->second : SceneryType::Unknown;
    }

    inline bool IsSpawnPoint(SceneryType type)
    {
        return type == SceneryType::InitialSpawnPoint || 
            type == SceneryType::RespawnPoint ||
            type == SceneryType::InvisibleRespawnPoint;
    }

    inline bool IsBoundary(SceneryType type)
    {
        return type == SceneryType::SafeBoundary
            || type == SceneryType::SoftSafeBoundary
            || type == SceneryType::KillBoundary
            || type == SceneryType::SoftKillBoundary;
    }

    inline bool IsPortableShield(SceneryType type)
    {
        return type == SceneryType::PortableShield;
    }
}

struct SceneryObject
{
    uintptr_t Base;
    SceneryType Type;

    std::optional<SpawnData> Spawn;
    std::optional<BoundaryData> Boundary;
    std::optional<PortableShieldData> PortableShield;
};