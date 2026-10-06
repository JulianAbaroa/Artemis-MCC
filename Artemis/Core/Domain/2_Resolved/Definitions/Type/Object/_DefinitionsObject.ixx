export module Resolved.Definitions.Type:Object;

import Common.Math.Type;
import std;

namespace
{
    using Vec3 = Common::Math::Type::Vec3;
}

export namespace Resolved::Definitions::Type::Object
{
    enum class Kind : std::uint16_t
    {
        Biped = 0x0000,
        Vehicle = 0x0001,
        Weapon = 0x0002,
        Equipment = 0x0003,
        Terminal = 0x0004,
        Projectile = 0x0005,
        Scenery = 0x0006,
        Machine = 0x0007,
        Control = 0x0008,
        SoundScenery = 0x0009,
        Crate = 0x000A,
        Creature = 0x000B,
        Giant = 0x000C,
        EffectScenery = 0x000D,

        Invalid = 0xFFFF,
    };

    enum class MultiplayerObjectKind : std::uint8_t
    {
        Ordinary = 0x00,
        Weapon = 0x01,
        Grenade = 0x02,
        Projectile = 0x03,
        Powerup = 0x04,
        Equipment = 0x05,
        AmmoPack = 0x06,
        LightLandVehicle = 0x07,
        HeavyLandVehicle = 0x08,
        FlyingVehicle = 0x09,
        Turret = 0x0A,
        Device = 0x0B,
        Teleporter2Way = 0x0C,
        TeleporterSender = 0x0D,
        TeleporterReceiver = 0x0E,
        PlayerSpawnLocation = 0x0F,
        PlayerRespawnZone = 0x10,
        SecondaryObjective = 0x11,
        PrimaryObjective = 0x12,
        NamedLocationArea = 0x13,
        DangerZone = 0x14,
        Fireteam1RespawnZone = 0x15,
        Fireteam2RespawnZone = 0x16,
        Fireteam3RespawnZone = 0x17,
        Fireteam4RespawnZone = 0x18,
        SafeVolume = 0x19,
        KillVolume = 0x1A,
        CinematicCameraPosition = 0x1B,

        Invalid = 0xFF,
    };

    struct MultiplayerObject
    {
        MultiplayerObjectKind Kind{ MultiplayerObjectKind::Invalid };
    };

    struct Object
    {
        std::string TagName{};

        // Classification
        Kind Kind{ Kind::Invalid };
        float BoundingRadius{};
        Vec3 BoundingOffset{};

        // Damage
        std::string CollisionDamageTagName{};
        std::string BrittleCollisionDamageTagName{};

        // Model
        std::string ModelTagName{};

        // Multiplayer
        std::vector<MultiplayerObject> MultiplayerObjects;

    };

    enum class DamageReportingType : std::uint8_t
    {
        GuardiansUnknown = 0x00,
        GuardiansDefault = 0x01,
        GuardiansScripting = 0x02,
        Suicide = 0x03,
        Magnum = 0x04,
        AssaultRifle = 0x05,
        DMR = 0x06,
        Shotgun = 0x07,
        SniperRifle = 0x08,
        RocketLauncher = 0x09,
        SpartanLaser = 0x0A,
        FragGrenade = 0x0B,
        GrenadeLauncher = 0x0C,
        PlasmaPistol = 0x0D,
        Needler = 0x0E,
        PlasmaRifle = 0x0F,
        PlasmaRepeater = 0x10,
        NeedleRifle = 0x11,
        Spiker = 0x12,
        PlasmaLauncher = 0x13,
        GravityHammer = 0x14,
        EnergySword = 0x15,
        PlasmaGrenade = 0x16,
        ConcussionRifle = 0x17,
        Ghost = 0x18,
        Revenant = 0x19,
        RevenantGunner = 0x1A,
        Wraith = 0x1B,
        WraithTurret = 0x1C,
        Banshee = 0x1D,
        BansheeBomb = 0x1E,
        Seraph = 0x1F,
        Mongoose = 0x20,
        Warthog = 0x21,
        WarthogChaingun = 0x22,
        WarthogGauss = 0x23,
        WarthogRocket = 0x24,
        Scorpion = 0x25,
        ScorpionTurret = 0x26,
        Falcon = 0x27,
        FalconGunner = 0x28,
        Falling = 0x29,
        Collision = 0x2A,
        Melee = 0x2B,
        Explosion = 0x2C,
        BirthdayExplosion = 0x2D,
        Flag = 0x2E,
        Bomb = 0x2F,
        BombExplosion = 0x30,
        Ball = 0x31,
        Teleporter = 0x32,
        TransferDamage = 0x33,
        ArmorLock = 0x34,
        TargetLocator = 0x35,
        HumanTurret = 0x36,
        PlasmaCannon = 0x37,
        PlasmaMortar = 0x38,
        PlasmaTurret = 0x39,
        ShadeTurret = 0x3A,
        Sabre = 0x3B,
        SMG = 0x3C,
        Carbine = 0x3D,
        BattleRifle = 0x3E,
        FocusRifle = 0x3F,
        FuelRod = 0x40,
        MissilePod = 0x41,
        BruteShot = 0x42,
        Flamethrower = 0x43,
        SentinelGun = 0x44,
        SpikeGrenade = 0x45,
        FirebombGrenade = 0x46,
        ElephantTurret = 0x47,
        Spectre = 0x48,
        SpectreGunner = 0x49,
        Tank = 0x4A,
        Chopper = 0x4B,
        Falcon_2 = 0x4C,
        Mantis = 0x4D,
        Prowler = 0x4E,
        SentinelBeam = 0x4F,
        SentinelRpg = 0x50,
        Tripmine = 0x51,

        Invalid = 0xFF,
    };
}