export module Resolved.Definitions.Type:Scnr;

import Common.Math.Type;
import std;

namespace
{
    using Common::Math::Type::Vec3;
}

export namespace Resolved::Definitions::Type::Scnr
{
    // Shape of a trigger volume.
    enum class TriggerVolumeKind : std::uint16_t
    {
        BoundingBox = 0x0000,
        Sector = 0x0001,

        Invalid = 0xFFFF,
    };

    // Effect of a soft ceiling.
    enum class SoftCeilingKind : std::uint16_t
    {
        Acceleration = 0x0000,
        SoftKill = 0x0001,
        SlipSurface = 0x0002,

        Invalid = 0xFFFF,
    };

    // Volume of the world that the scenario uses to trigger things.
    // note: A bounding box starts at Position and spans Extents along Forward, the left axis and Up.
    // Not confirmed yet.
    struct TriggerVolume
    {
        // Name as a string id. There is no string table reader yet.
        std::uint32_t Name{};

        TriggerVolumeKind Kind{ TriggerVolumeKind::Invalid };

        Vec3 Position{};
        Vec3 Forward{};
        Vec3 Up{};
        Vec3 Extents{};

        // Only valid for a sector.
        float ZSink{};
        std::vector<Vec3> SectorPoints{};
    };

    // Trigger volume that kills, or makes safe, whatever is inside.
    struct BoundaryTrigger
    {
        // Index in the trigger volumes of the scenario.
        std::int16_t TriggerVolumeIndex{ -1 };

        bool DontKillImmediately{ false };
        bool OnlyKillPlayers{ false };
    };

    // Ceiling that the scenario places on the map.
    // note: The scenario holds only its name, kind and flags. Where the surfaces are is not known yet.
    struct SoftCeiling
    {
        // Name as a string id. There is no string table reader yet.
        std::uint32_t Name{};

        SoftCeilingKind Kind{ SoftCeilingKind::Invalid };

        bool IgnoreBipeds{ false };
        bool IgnoreVehicles{ false };
        bool IgnoreCamera{ false };
        bool IgnoreHugeVehicles{ false };
    };

    // Holds the limits of the map that the scenario defines.
    struct Scnr
    {
        std::string TagName{};

        std::vector<TriggerVolume> TriggerVolumes{};
        std::vector<BoundaryTrigger> KillTriggers{};
        std::vector<BoundaryTrigger> SafeZoneTriggers{};
        std::vector<SoftCeiling> SoftCeilings{};
    };
}