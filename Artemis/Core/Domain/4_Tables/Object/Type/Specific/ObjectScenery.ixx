export module Tables.Object.Type:Scenery;

import Common.Tag.Type;
import std;

export namespace Tables::Object::Type::Scenery
{
    enum class Kind : std::uint8_t
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

    inline Kind ResolveSceneryType(const std::string& tagName)
    {
    	namespace Name = Common::Tag::Type;

        static const std::unordered_map<std::string, Kind> map =
        {
            // Spawns.
            { Name::Spawn::k_InvisibleRespawnPoint,Kind::InvisibleRespawnPoint  },
            { Name::Spawn::k_InitialSpawnPoint,    Kind::InitialSpawnPoint      },
            { Name::Spawn::k_RespawnPoint,         Kind::RespawnPoint           },

            // Boundaries
            { Name::Boundary::k_SafeBoundary,      Kind::SafeBoundary           },
            { Name::Boundary::k_SoftSafeBoundary,  Kind::SoftSafeBoundary       },
            { Name::Boundary::k_KillBoundary,      Kind::KillBoundary           },
            { Name::Boundary::k_SoftKillBoundary,  Kind::SoftKillBoundary       },

            { Name::Shield::k_PortableShield,      Kind::PortableShield         },
        };

        auto it = map.find(tagName);
        return it != map.end() ? it->second : Kind::Unknown;
    }

    struct Scenery
    {
        std::uintptr_t Base{};
        Kind Kind{};
    };
}