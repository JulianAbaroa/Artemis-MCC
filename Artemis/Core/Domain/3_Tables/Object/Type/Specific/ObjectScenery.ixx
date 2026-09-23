export module Tables.Object.Type:Scenery;

import Common.Tag.Name;
import Common.Team.Type;
import Common.ZoneShape.Type;
import std;

namespace
{
    using Team = Common::Team::Type::Team;
    using ZoneShape = Common::ZoneShape::Type::ZoneShape;
}

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

    namespace Spawn
    {
        struct Spawn
        {
            Team Team{};
        };
    }

    namespace Boundary
    {
        struct Boundary
        {
            ZoneShape Shape{};
            Team Team{};
        };
    }

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
    	namespace Name = Common::Tag::Name;
    
        static const std::unordered_map<std::string, Kind> map =
        {
            // Spawns.
            { Name::Spawn::InvisibleRespawnPoint,Kind::InvisibleRespawnPoint  },
            { Name::Spawn::InitialSpawnPoint,    Kind::InitialSpawnPoint      },
            { Name::Spawn::RespawnPoint,         Kind::RespawnPoint           },

            // Boundaries
            { Name::Boundary::SafeBoundary,      Kind::SafeBoundary           },
            { Name::Boundary::SoftSafeBoundary,  Kind::SoftSafeBoundary       },
            { Name::Boundary::KillBoundary,      Kind::KillBoundary           },
            { Name::Boundary::SoftKillBoundary,  Kind::SoftKillBoundary       },

            { Name::Shield::PortableShield,      Kind::PortableShield         },
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

        std::optional<Spawn::Spawn> Spawn{};
        std::optional<Boundary::Boundary> Boundary{};
        std::optional<PortableShield::PortableShield> PortableShield{};
    };
}