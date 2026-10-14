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

    namespace PortableShield
    {
        struct PortableShield
        {
            std::uint16_t RegenerationDelay{};
            float Health{};
        };
    }

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

    inline bool IsSpawnPoint(Kind kind)
    {
        return kind == Kind::InitialSpawnPoint
            || kind == Kind::RespawnPoint
            || kind == Kind::InvisibleRespawnPoint;
    }

    inline bool IsBoundary(Kind kind)
    {
        return kind == Kind::SafeBoundary
            || kind == Kind::SoftSafeBoundary
            || kind == Kind::KillBoundary
            || kind == Kind::SoftKillBoundary;
    }

    inline bool IsPortableShield(Kind kind)
    {
        return kind == Kind::PortableShield;
    }

    struct Scenery
    {
        std::uintptr_t Base{};
        Kind Kind{};

        std::optional<PortableShield::PortableShield> PortableShield{};
    };
}