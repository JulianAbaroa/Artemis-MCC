export module Tables.Object.Type:Scenery;

import Common.Tag.Type;
import std;

namespace
{
    using Common::Tag::Type::Spawn::k_InvisibleRespawnPoint;
    using Common::Tag::Type::Boundary::k_SoftSafeBoundary;
    using Common::Tag::Type::Boundary::k_SoftKillBoundary;
    using Common::Tag::Type::Spawn::k_InitialSpawnPoint;
    using Common::Tag::Type::Boundary::k_KillBoundary;
    using Common::Tag::Type::Shield::k_PortableShield;
    using Common::Tag::Type::Boundary::k_SafeBoundary;
    using Common::Tag::Type::Spawn::k_RespawnPoint;
}

export namespace Tables::Object::Type::Scenery
{
    enum class SceneryKind : std::uint8_t
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

    inline auto ResolveSceneryType(const std::string& tagName) -> SceneryKind
    {
        static const std::unordered_map<std::string, SceneryKind> map =
        {
            // Spawns.
            { k_InvisibleRespawnPoint,SceneryKind::InvisibleRespawnPoint  },
            { k_InitialSpawnPoint,    SceneryKind::InitialSpawnPoint      },
            { k_RespawnPoint,         SceneryKind::RespawnPoint           },

            // Boundaries
            { k_SafeBoundary,      SceneryKind::SafeBoundary           },
            { k_SoftSafeBoundary,  SceneryKind::SoftSafeBoundary       },
            { k_KillBoundary,      SceneryKind::KillBoundary           },
            { k_SoftKillBoundary,  SceneryKind::SoftKillBoundary       },

            { k_PortableShield,      SceneryKind::PortableShield         },
        };

        auto it = map.find(tagName);
        return it != map.end() ? it->second : SceneryKind::Unknown;
    }

    struct Scenery
    {
        std::uintptr_t Base{};
        SceneryKind Kind{};
    };
}