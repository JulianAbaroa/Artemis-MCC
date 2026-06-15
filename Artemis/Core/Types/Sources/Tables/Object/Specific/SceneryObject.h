#pragma once

#include "Core/Types/Team.h"
#include "Core/Types/TagName.h"
#include "Core/Types/ZoneShape.h"
#include <unordered_map>
#include <optional>

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
        static const std::unordered_map<std::string, SceneryType> map =
        {
            { TagName::Spawn::InvisibleRespawnPoint, SceneryType::InvisibleRespawnPoint  },
            { TagName::Spawn::InitialSpawnPoint,     SceneryType::InitialSpawnPoint      },
            { TagName::Spawn::RespawnPoint,          SceneryType::RespawnPoint           },
            { TagName::Boundary::SafeBoundary,       SceneryType::SafeBoundary           },
            { TagName::Boundary::SoftSafeBoundary,   SceneryType::SoftSafeBoundary       },
            { TagName::Boundary::KillBoundary,       SceneryType::KillBoundary           },
            { TagName::Boundary::SoftKillBoundary,   SceneryType::SoftKillBoundary       },
            { TagName::Shield::PortableShield,       SceneryType::PortableShield         },
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